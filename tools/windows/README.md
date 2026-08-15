# Windows Bring-up Tools

Windows owns the runtime I/O side of the development workflow. WSL owns source, build, test, and patch validation.

## Requirements

```powershell
py -m pip install pyserial
```

## GUI mode

```powershell
py tools\windows\serial_tool.py --port COM13
```

## CLI mode

```powershell
py tools\windows\serial_tool.py --port COM13 `
  "param get control.enable_request"
```

## Command scenarios

```powershell
py tools\windows\serial_tool.py --port COM13 `
  --file tools\windows\cli\gate_test.txt `
  --log gate_test.log
```

Only one process may own the COM port at a time. Disconnect MobaXterm or any other serial terminal before starting `serial_tool.py`.

Default transport settings are 115200 baud, CR command termination, and 2 ms pacing between transmitted characters.
