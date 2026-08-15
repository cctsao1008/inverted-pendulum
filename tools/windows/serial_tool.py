#!/usr/bin/env python3
"""
serial_tool.py
Windows-oriented serial bring-up tool for the inverted-pendulum project.

Modes
-----
GUI mode (default):
    py serial_tool.py

GUI mode with initial port:
    py serial_tool.py --port COM5

CLI mode:
    py serial_tool.py --port COM5 "param get control.enable_request"

CLI mode with multiple commands:
    py serial_tool.py --port COM5 ^
        "param set control.enable_request 1" ^
        "param get control.enable_request"

CLI mode with command file:
    py serial_tool.py --port COM5 --file gate_test.txt

List ports:
    py serial_tool.py --list-ports

Requirements
------------
    py -m pip install pyserial

Notes
-----
- Intended to run directly on Windows and access COM ports.
- MobaXterm or any other serial terminal must release the COM port first.
- TX is paced character-by-character to reduce dropped characters.
"""

from __future__ import annotations

import argparse
import queue
import sys
import threading
import time
import tkinter as tk
from datetime import datetime
from pathlib import Path
from tkinter import filedialog, messagebox, ttk

import serial
from serial.tools import list_ports


APP_TITLE = "Inverted Pendulum Serial Tool"

DEFAULT_BAUD = 115200
DEFAULT_CHAR_DELAY_MS = 2
DEFAULT_COMMAND_DELAY_MS = 150
DEFAULT_RESPONSE_TIMEOUT_MS = 500
DEFAULT_QUIET_MS = 120


# ---------------------------------------------------------------------------
# Shared transport / command helpers
# ---------------------------------------------------------------------------

def available_ports() -> list[str]:
    return [p.device for p in list_ports.comports()]


def load_command_file(path: str | Path) -> list[str]:
    commands: list[str] = []

    with open(path, "r", encoding="utf-8-sig") as fp:
        for raw in fp:
            line = raw.strip()
            if not line or line.startswith("#"):
                continue
            commands.append(line)

    return commands


def line_ending_bytes(name: str) -> bytes:
    if name == "lf":
        return b"\n"
    if name == "crlf":
        return b"\r\n"
    return b"\r"


def send_paced(
    ser: serial.Serial,
    command: str,
    char_delay_ms: int,
    line_ending: bytes = b"\r",
) -> None:
    delay_s = max(0, int(char_delay_ms)) / 1000.0
    payload = command.encode("utf-8") + line_ending

    for byte in payload:
        ser.write(bytes([byte]))
        ser.flush()
        if delay_s:
            time.sleep(delay_s)


def read_response(
    ser: serial.Serial,
    timeout_ms: int,
    quiet_ms: int,
) -> str:
    deadline = time.monotonic() + max(0, timeout_ms) / 1000.0
    quiet_s = max(0, quiet_ms) / 1000.0

    last_rx: float | None = None
    buf = bytearray()

    while time.monotonic() < deadline:
        waiting = ser.in_waiting
        if waiting:
            data = ser.read(waiting)
            if data:
                buf.extend(data)
                last_rx = time.monotonic()
                continue

        now = time.monotonic()
        if last_rx is not None and quiet_s > 0 and (now - last_rx) >= quiet_s:
            break

        time.sleep(0.01)

    return buf.decode("utf-8", errors="replace")


def open_serial(port: str, baud: int) -> serial.Serial:
    return serial.Serial(
        port=port,
        baudrate=baud,
        bytesize=serial.EIGHTBITS,
        parity=serial.PARITY_NONE,
        stopbits=serial.STOPBITS_ONE,
        timeout=0.05,
        write_timeout=1.0,
    )


# ---------------------------------------------------------------------------
# CLI mode
# ---------------------------------------------------------------------------

class Tee:
    def __init__(self, logfile: str | None) -> None:
        self.fp = None
        if logfile:
            self.fp = open(logfile, "a", encoding="utf-8", newline="")

    def write(self, text: str) -> None:
        sys.stdout.write(text)
        sys.stdout.flush()

        if self.fp:
            self.fp.write(text)
            self.fp.flush()

    def close(self) -> None:
        if self.fp:
            self.fp.close()


def run_cli(args: argparse.Namespace) -> int:
    ports = available_ports()

    if args.list_ports:
        if ports:
            print("\n".join(ports))
        else:
            print("No serial ports found.")
        return 0

    if not args.port:
        print("No --port specified.")
        print()
        if ports:
            print("Available ports:")
            for port in ports:
                print(f"  {port}")
        else:
            print("No serial ports found.")
        print()
        print('Example: py serial_tool.py --port COM5 "help"')
        return 2

    commands: list[str] = []

    for path in args.file:
        try:
            commands.extend(load_command_file(path))
        except OSError as exc:
            print(f"[ERROR] Cannot read command file {path}: {exc}", file=sys.stderr)
            return 3

    commands.extend(args.commands)

    if not commands:
        print("[ERROR] No commands specified for CLI mode.", file=sys.stderr)
        return 4

    tee = Tee(args.log)

    try:
        ser = open_serial(args.port, args.baud)
    except (serial.SerialException, OSError) as exc:
        tee.write(f"[ERROR] Cannot open {args.port}: {exc}\n")
        tee.write("Close MobaXterm or any other program using this COM port.\n")
        tee.close()
        return 5

    tee.write(
        f"[HOST] Connected {args.port} @ {args.baud}, "
        f"char_delay={args.char_delay_ms}ms\n"
    )

    terminator = line_ending_bytes(args.line_ending)

    try:
        ser.reset_input_buffer()

        for index, command in enumerate(commands, start=1):
            if not args.no_echo:
                tee.write(f"[HOST->MCU] {command}\n")

            send_paced(
                ser,
                command,
                args.char_delay_ms,
                terminator,
            )

            response = read_response(
                ser,
                args.response_timeout_ms,
                args.quiet_ms,
            )

            if response:
                tee.write(response)
                if not response.endswith("\n"):
                    tee.write("\n")
            else:
                tee.write("[HOST] No response captured.\n")

            if index != len(commands):
                time.sleep(max(0, args.command_delay_ms) / 1000.0)

    except KeyboardInterrupt:
        tee.write("\n[HOST] Interrupted.\n")
        return 130
    except (serial.SerialException, OSError) as exc:
        tee.write(f"\n[SERIAL ERROR] {exc}\n")
        return 6
    finally:
        try:
            ser.close()
        except Exception:
            pass

        tee.write("[HOST] Serial port closed.\n")
        tee.close()

    return 0


# ---------------------------------------------------------------------------
# GUI mode
# ---------------------------------------------------------------------------

class GuiSerialWorker:
    def __init__(self, rx_queue: queue.Queue[str]) -> None:
        self.rx_queue = rx_queue
        self.ser: serial.Serial | None = None
        self.stop_event = threading.Event()
        self.tx_lock = threading.Lock()

    @property
    def connected(self) -> bool:
        return self.ser is not None and self.ser.is_open

    def connect(self, port: str, baud: int) -> None:
        self.disconnect()

        self.ser = open_serial(port, baud)
        self.stop_event.clear()

        threading.Thread(
            target=self._reader_loop,
            name="serial-reader",
            daemon=True,
        ).start()

    def disconnect(self) -> None:
        self.stop_event.set()

        if self.ser is not None:
            try:
                self.ser.close()
            except Exception:
                pass

        self.ser = None

    def _reader_loop(self) -> None:
        pending = bytearray()

        while not self.stop_event.is_set():
            ser = self.ser
            if ser is None or not ser.is_open:
                return

            try:
                data = ser.read(512)
            except (serial.SerialException, OSError) as exc:
                self.rx_queue.put(f"\n[SERIAL ERROR] {exc}\n")
                self.disconnect()
                return

            if not data:
                continue

            pending.extend(data)

            while b"\n" in pending:
                raw, _, pending = pending.partition(b"\n")
                text = raw.rstrip(b"\r").decode("utf-8", errors="replace")
                self.rx_queue.put(text + "\n")

        if pending:
            self.rx_queue.put(pending.decode("utf-8", errors="replace"))

    def send(
        self,
        command: str,
        char_delay_ms: int,
        line_ending: bytes,
    ) -> None:
        if not self.connected:
            raise RuntimeError("Serial port is not connected.")

        with self.tx_lock:
            ser = self.ser
            if ser is None:
                raise RuntimeError("Serial port is not connected.")

            send_paced(
                ser,
                command,
                char_delay_ms,
                line_ending,
            )


class SerialGuiApp:
    def __init__(
        self,
        root: tk.Tk,
        initial_port: str | None,
        initial_baud: int,
    ) -> None:
        self.root = root
        self.root.title(f"{APP_TITLE} - Command")
        self.root.geometry("820x320")
        self.root.minsize(680, 280)

        self.log_window = tk.Toplevel(root)
        self.log_window.title(f"{APP_TITLE} - UART Log")
        self.log_window.geometry("1180x720")
        self.log_window.minsize(760, 440)

        self.rx_queue: queue.Queue[str] = queue.Queue()
        self.worker = GuiSerialWorker(self.rx_queue)

        self.history: list[str] = []
        self.history_index = 0

        self.port_var = tk.StringVar(value=initial_port or "")
        self.baud_var = tk.StringVar(value=str(initial_baud))
        self.char_delay_var = tk.IntVar(value=DEFAULT_CHAR_DELAY_MS)
        self.command_delay_var = tk.IntVar(value=DEFAULT_COMMAND_DELAY_MS)
        self.line_ending_var = tk.StringVar(value="cr")
        self.command_var = tk.StringVar()
        self.status_var = tk.StringVar(value="Disconnected")

        self._build_command_window()
        self._build_log_window()
        self.refresh_ports(select_if_empty=True)

        self.root.protocol("WM_DELETE_WINDOW", self.close)
        self.log_window.protocol("WM_DELETE_WINDOW", self.log_window.withdraw)

        self.root.after(30, self._drain_rx_queue)

    def _build_command_window(self) -> None:
        outer = ttk.Frame(self.root, padding=10)
        outer.pack(fill="both", expand=True)

        conn = ttk.LabelFrame(outer, text="Serial", padding=8)
        conn.pack(fill="x")

        ttk.Label(conn, text="Port").grid(row=0, column=0, sticky="w")

        self.port_combo = ttk.Combobox(
            conn,
            textvariable=self.port_var,
            width=18,
            state="normal",
        )
        self.port_combo.grid(row=0, column=1, padx=(5, 10), sticky="ew")

        ttk.Button(
            conn,
            text="Refresh",
            command=self.refresh_ports,
        ).grid(row=0, column=2, padx=(0, 10))

        ttk.Label(conn, text="Baud").grid(row=0, column=3, sticky="w")

        ttk.Entry(
            conn,
            textvariable=self.baud_var,
            width=10,
        ).grid(row=0, column=4, padx=(5, 10))

        self.connect_button = ttk.Button(
            conn,
            text="Connect",
            command=self.toggle_connection,
        )
        self.connect_button.grid(row=0, column=5)

        conn.columnconfigure(1, weight=1)

        options = ttk.Frame(outer, padding=(0, 8, 0, 4))
        options.pack(fill="x")

        ttk.Label(options, textvariable=self.status_var).pack(side="left")

        ttk.Label(options, text="TX char delay").pack(
            side="left", padx=(18, 5)
        )
        ttk.Spinbox(
            options,
            from_=0,
            to=50,
            textvariable=self.char_delay_var,
            width=5,
        ).pack(side="left")
        ttk.Label(options, text="ms").pack(side="left", padx=(3, 10))

        ttk.Label(options, text="Command delay").pack(side="left", padx=(8, 5))
        ttk.Spinbox(
            options,
            from_=0,
            to=5000,
            textvariable=self.command_delay_var,
            width=6,
        ).pack(side="left")
        ttk.Label(options, text="ms").pack(side="left", padx=(3, 10))

        ttk.Label(options, text="Line ending").pack(side="left", padx=(8, 5))
        ttk.Combobox(
            options,
            textvariable=self.line_ending_var,
            values=["cr", "lf", "crlf"],
            width=6,
            state="readonly",
        ).pack(side="left")

        command_frame = ttk.LabelFrame(outer, text="Command", padding=8)
        command_frame.pack(fill="x", pady=(4, 0))

        self.command_entry = ttk.Entry(
            command_frame,
            textvariable=self.command_var,
            font=("Consolas", 11),
        )
        self.command_entry.grid(row=0, column=0, sticky="ew", padx=(0, 8))
        self.command_entry.bind("<Return>", self._send_from_event)
        self.command_entry.bind("<Up>", self._history_up)
        self.command_entry.bind("<Down>", self._history_down)

        ttk.Button(
            command_frame,
            text="Send",
            command=self.send_current,
        ).grid(row=0, column=1)

        command_frame.columnconfigure(0, weight=1)

        quick = ttk.Frame(outer, padding=(0, 8, 0, 0))
        quick.pack(fill="x")

        presets = [
            ("help", "help"),
            ("status", "status"),
            ("Gate get", "param get control.enable_request"),
            ("Gate ON", "param set control.enable_request 1"),
            ("Gate OFF", "param set control.enable_request 0"),
        ]

        for label, command in presets:
            ttk.Button(
                quick,
                text=label,
                command=lambda c=command: self.send_command(c),
            ).pack(side="left", padx=(0, 6))

        ttk.Button(
            quick,
            text="Run command file...",
            command=self.run_command_file,
        ).pack(side="right")

        ttk.Button(
            quick,
            text="Show log",
            command=self.show_log_window,
        ).pack(side="right", padx=(0, 8))

    def _build_log_window(self) -> None:
        toolbar = ttk.Frame(self.log_window, padding=8)
        toolbar.pack(fill="x")

        ttk.Button(
            toolbar,
            text="Clear",
            command=self.clear_log,
        ).pack(side="left")

        ttk.Button(
            toolbar,
            text="Save log...",
            command=self.save_log,
        ).pack(side="left", padx=(6, 0))

        ttk.Button(
            toolbar,
            text="Show command window",
            command=self.show_command_window,
        ).pack(side="right")

        frame = ttk.Frame(self.log_window, padding=(8, 0, 8, 8))
        frame.pack(fill="both", expand=True)

        self.log_text = tk.Text(
            frame,
            wrap="none",
            font=("Consolas", 10),
            undo=False,
        )

        yscroll = ttk.Scrollbar(
            frame,
            orient="vertical",
            command=self.log_text.yview,
        )
        xscroll = ttk.Scrollbar(
            frame,
            orient="horizontal",
            command=self.log_text.xview,
        )

        self.log_text.configure(
            yscrollcommand=yscroll.set,
            xscrollcommand=xscroll.set,
        )

        self.log_text.grid(row=0, column=0, sticky="nsew")
        yscroll.grid(row=0, column=1, sticky="ns")
        xscroll.grid(row=1, column=0, sticky="ew")

        frame.rowconfigure(0, weight=1)
        frame.columnconfigure(0, weight=1)

    def refresh_ports(self, select_if_empty: bool = False) -> None:
        ports = available_ports()
        self.port_combo["values"] = ports

        if select_if_empty and not self.port_var.get() and ports:
            self.port_var.set(ports[0])

    def toggle_connection(self) -> None:
        if self.worker.connected:
            self.worker.disconnect()
            self.connect_button.configure(text="Connect")
            self.status_var.set("Disconnected")
            return

        port = self.port_var.get().strip()

        if not port:
            messagebox.showerror(APP_TITLE, "Select or enter a serial port.")
            return

        try:
            baud = int(self.baud_var.get())
            if baud <= 0:
                raise ValueError
        except ValueError:
            messagebox.showerror(
                APP_TITLE,
                "Baud rate must be a positive integer.",
            )
            return

        try:
            self.worker.connect(port, baud)
        except (serial.SerialException, OSError) as exc:
            messagebox.showerror(
                APP_TITLE,
                f"Cannot open {port}:\n\n{exc}\n\n"
                "Close MobaXterm or any other program using this COM port.",
            )
            return

        self.connect_button.configure(text="Disconnect")
        self.status_var.set(f"Connected: {port} @ {baud}")

        self._append_log(
            f"\n[HOST] Connected {port} @ {baud} "
            f"at {datetime.now().isoformat(timespec='seconds')}\n"
        )

        self.command_entry.focus_set()

    def _send_from_event(self, _event: tk.Event) -> str:
        self.send_current()
        return "break"

    def send_current(self) -> None:
        command = self.command_var.get().strip()
        if not command:
            return

        self.command_var.set("")
        self.send_command(command)

    def send_command(self, command: str) -> None:
        command = command.strip()

        if not command:
            return

        if not self.worker.connected:
            messagebox.showwarning(APP_TITLE, "Serial port is not connected.")
            return

        if not self.history or self.history[-1] != command:
            self.history.append(command)

        self.history_index = len(self.history)

        self._append_log(f"[HOST->MCU] {command}\n")

        threading.Thread(
            target=self._send_worker,
            args=(command,),
            daemon=True,
        ).start()

    def _send_worker(self, command: str) -> None:
        try:
            self.worker.send(
                command,
                int(self.char_delay_var.get()),
                line_ending_bytes(self.line_ending_var.get()),
            )
        except Exception as exc:
            self.rx_queue.put(f"[TX ERROR] {exc}\n")

    def run_command_file(self) -> None:
        if not self.worker.connected:
            messagebox.showwarning(APP_TITLE, "Serial port is not connected.")
            return

        path = filedialog.askopenfilename(
            title="Select command file",
            filetypes=[
                ("Text files", "*.txt"),
                ("All files", "*.*"),
            ],
        )

        if not path:
            return

        try:
            commands = load_command_file(path)
        except OSError as exc:
            messagebox.showerror(
                APP_TITLE,
                f"Cannot read command file:\n{exc}",
            )
            return

        if not commands:
            messagebox.showinfo(
                APP_TITLE,
                "The command file contains no commands.",
            )
            return

        threading.Thread(
            target=self._run_commands_worker,
            args=(commands,),
            daemon=True,
        ).start()

    def _run_commands_worker(self, commands: list[str]) -> None:
        for command in commands:
            if not self.worker.connected:
                self.rx_queue.put(
                    "[HOST] Command file stopped: serial disconnected.\n"
                )
                return

            self.rx_queue.put(f"[HOST->MCU] {command}\n")

            try:
                self.worker.send(
                    command,
                    int(self.char_delay_var.get()),
                    line_ending_bytes(self.line_ending_var.get()),
                )
            except Exception as exc:
                self.rx_queue.put(f"[TX ERROR] {exc}\n")
                return

            time.sleep(
                max(0, int(self.command_delay_var.get())) / 1000.0
            )

        self.rx_queue.put("[HOST] Command file complete.\n")

    def _history_up(self, _event: tk.Event) -> str:
        if self.history:
            self.history_index = max(0, self.history_index - 1)
            self.command_var.set(self.history[self.history_index])
            self.command_entry.icursor("end")
        return "break"

    def _history_down(self, _event: tk.Event) -> str:
        if self.history:
            self.history_index = min(
                len(self.history),
                self.history_index + 1,
            )

            if self.history_index == len(self.history):
                self.command_var.set("")
            else:
                self.command_var.set(self.history[self.history_index])

            self.command_entry.icursor("end")

        return "break"

    def _drain_rx_queue(self) -> None:
        try:
            while True:
                self._append_log(self.rx_queue.get_nowait())
        except queue.Empty:
            pass

        if self.root.winfo_exists():
            self.root.after(30, self._drain_rx_queue)

    def _append_log(self, text: str) -> None:
        self.log_text.insert("end", text)
        self.log_text.see("end")

        try:
            line_count = int(
                self.log_text.index("end-1c").split(".")[0]
            )
            if line_count > 20000:
                self.log_text.delete("1.0", "5000.0")
        except Exception:
            pass

    def clear_log(self) -> None:
        self.log_text.delete("1.0", "end")

    def save_log(self) -> None:
        default_name = datetime.now().strftime(
            "uart_%Y%m%d_%H%M%S.log"
        )

        path = filedialog.asksaveasfilename(
            title="Save UART log",
            initialfile=default_name,
            defaultextension=".log",
            filetypes=[
                ("Log files", "*.log"),
                ("Text files", "*.txt"),
                ("All files", "*.*"),
            ],
        )

        if not path:
            return

        try:
            with open(path, "w", encoding="utf-8", newline="") as fp:
                fp.write(self.log_text.get("1.0", "end-1c"))
        except OSError as exc:
            messagebox.showerror(
                APP_TITLE,
                f"Cannot save log:\n{exc}",
            )

    def show_log_window(self) -> None:
        self.log_window.deiconify()
        self.log_window.lift()

    def show_command_window(self) -> None:
        self.root.deiconify()
        self.root.lift()

    def close(self) -> None:
        self.worker.disconnect()

        try:
            self.log_window.destroy()
        except tk.TclError:
            pass

        self.root.destroy()


# ---------------------------------------------------------------------------
# Argument parsing / mode selection
# ---------------------------------------------------------------------------

def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description=(
            "Windows serial GUI + paced CLI command tool."
        )
    )

    parser.add_argument(
        "commands",
        nargs="*",
        help=(
            "CLI commands. If one or more commands are supplied, "
            "CLI mode is selected automatically."
        ),
    )

    parser.add_argument(
        "--port",
        help="Serial port, e.g. COM5.",
    )

    parser.add_argument(
        "--baud",
        type=int,
        default=DEFAULT_BAUD,
        help=f"Baud rate (default: {DEFAULT_BAUD}).",
    )

    parser.add_argument(
        "--char-delay-ms",
        type=int,
        default=DEFAULT_CHAR_DELAY_MS,
        help=(
            "CLI TX character delay "
            f"(default: {DEFAULT_CHAR_DELAY_MS} ms)."
        ),
    )

    parser.add_argument(
        "--command-delay-ms",
        type=int,
        default=DEFAULT_COMMAND_DELAY_MS,
        help=(
            "CLI delay between commands "
            f"(default: {DEFAULT_COMMAND_DELAY_MS} ms)."
        ),
    )

    parser.add_argument(
        "--response-timeout-ms",
        type=int,
        default=DEFAULT_RESPONSE_TIMEOUT_MS,
        help=(
            "CLI maximum response wait "
            f"(default: {DEFAULT_RESPONSE_TIMEOUT_MS} ms)."
        ),
    )

    parser.add_argument(
        "--quiet-ms",
        type=int,
        default=DEFAULT_QUIET_MS,
        help=(
            "CLI RX quiet time before response is considered complete "
            f"(default: {DEFAULT_QUIET_MS} ms)."
        ),
    )

    parser.add_argument(
        "--file",
        action="append",
        default=[],
        help=(
            "CLI command file. May be specified more than once. "
            "Supplying --file selects CLI mode."
        ),
    )

    parser.add_argument(
        "--log",
        help="CLI mode: append session output to this file.",
    )

    parser.add_argument(
        "--no-echo",
        action="store_true",
        help="CLI mode: do not print [HOST->MCU] lines.",
    )

    parser.add_argument(
        "--line-ending",
        choices=["cr", "lf", "crlf"],
        default="cr",
        help="CLI line ending (default: cr).",
    )

    parser.add_argument(
        "--list-ports",
        action="store_true",
        help="List available serial ports and exit.",
    )

    parser.add_argument(
        "--gui",
        action="store_true",
        help=(
            "Force GUI mode even if other CLI-mode arguments are present."
        ),
    )

    return parser.parse_args()


def main() -> int:
    args = parse_args()

    if args.list_ports:
        return run_cli(args)

    cli_requested = bool(args.commands or args.file)

    if args.gui or not cli_requested:
        root = tk.Tk()
        SerialGuiApp(
            root,
            initial_port=args.port,
            initial_baud=args.baud,
        )
        root.mainloop()
        return 0

    return run_cli(args)


if __name__ == "__main__":
    raise SystemExit(main())
