param(
    [string]$Compiler = 'gcc',
    [string]$Python = 'python'
)
$ErrorActionPreference = 'Stop'
Push-Location (Join-Path $PSScriptRoot '..')
try {
    New-Item -ItemType Directory -Force tests/.build | Out-Null
    & $Compiler -std=c11 -Wall -Wextra -Werror -Wno-unused-parameter -DVEHICLE_LEGACY_UART=0 -I tests/stubs -I Core/Inc -I USB_Log -I hardware/AT24C02 tests/test_review_fixes.c -o tests/.build/test_review_fixes.exe
    if ($LASTEXITCODE) { throw 'Review regression build failed' }
    & tests/.build/test_review_fixes.exe
    if ($LASTEXITCODE) { throw 'Review regression failed' }
    & $Compiler -std=c11 -Wall -Wextra -Werror tests/test_control_gate.c framework/control_gate.c -o tests/.build/test_control_gate.exe
    if ($LASTEXITCODE) { throw 'Control gate build failed' }
    & tests/.build/test_control_gate.exe
    if ($LASTEXITCODE) { throw 'Control gate tests failed' }
    & $Compiler -std=c11 -Wall -Wextra -Werror tests/test_car_protocol.c framework/car_protocol.c framework/car_link.c framework/control_gate.c -o tests/.build/test_car_protocol.exe
    if ($LASTEXITCODE) { throw 'Protocol build failed' }
    & tests/.build/test_car_protocol.exe
    if ($LASTEXITCODE) { throw 'Protocol tests failed' }
    & $Compiler -std=c11 -Wall -Wextra -Werror -I Core/Inc tests/test_vehicle.c Core/Src/vehicle_core.c -lm -o tests/.build/test_vehicle.exe
    if ($LASTEXITCODE) { throw 'Vehicle build failed' }
    & tests/.build/test_vehicle.exe
    if ($LASTEXITCODE) { throw 'Vehicle tests failed' }
    & $Compiler -std=c11 -Wall -Wextra -Werror -I Core/Inc tests/test_vehicle_p2.c Core/Src/vehicle_core.c -lm -o tests/.build/test_vehicle_p2.exe
    if ($LASTEXITCODE) { throw 'Vehicle P2 build failed' }
    foreach ($case in @('timeout','resync')) {
        & tests/.build/test_vehicle_p2.exe $case
        if ($LASTEXITCODE) { throw "Vehicle P2 $case failed" }
    }
    & $Compiler -std=c11 -Wall -Wextra -Werror -I tests/log_stubs -I tests/stubs -I Core/Inc -I USB_Log tests/test_vehicle_log.c Core/Src/vehicle_core.c -lm -o tests/.build/test_vehicle_log.exe
    if ($LASTEXITCODE) { throw 'Vehicle log build failed' }
    & tests/.build/test_vehicle_log.exe
    if ($LASTEXITCODE) { throw 'Vehicle log test failed' }
    & $Compiler -std=c11 -Wall -Wextra -Werror -DVEHICLE_DISPLAY_HOST_TEST -I Core/Inc tests/test_vehicle_display.c Core/Src/vehicle_display.c -lm -o tests/.build/test_vehicle_display.exe
    if ($LASTEXITCODE) { throw 'Display test build failed' }
    & tests/.build/test_vehicle_display.exe
    if ($LASTEXITCODE) { throw 'Display test failed' }
    & $Compiler -std=c11 -Wall -Wextra -Werror -Wno-unused-parameter -I tests/stubs -I Core/Inc -I USB_Log tests/test_uart7_legacy.c Core/Src/vehicle_core.c -lm -o tests/.build/test_uart7_legacy.exe
    if ($LASTEXITCODE) { throw 'Legacy UART integration build failed' }
    & tests/.build/test_uart7_legacy.exe
    if ($LASTEXITCODE) { throw 'Legacy UART integration tests failed' }
    foreach ($gate in @(0,1)) {
        & $Compiler -std=c11 -Wall -Wextra -Werror -DSERVO_BENCH_MODE=0 "-DVEHICLE_OUTPUTS_ALLOWED=$gate" -I tests/port_stubs -I Core/Inc tests/test_vehicle_ports.c Core/Src/vehicle_ports.c -o tests/.build/test_vehicle_ports.exe
        if ($LASTEXITCODE) { throw 'Port simulation build failed' }
        & tests/.build/test_vehicle_ports.exe
        if ($LASTEXITCODE) { throw 'Port simulation failed' }
    }
    & $Compiler -std=c11 -Wall -Wextra -Werror -I Core/Inc tests/test_racing_metadata.c Core/Src/vehicle_core.c -lm -o tests/.build/test_racing_metadata.exe
    if ($LASTEXITCODE) { throw 'Racing metadata build failed' }
    & tests/.build/test_racing_metadata.exe
    if ($LASTEXITCODE) { throw 'Racing metadata failed' }
    & $Compiler -std=c11 -Wall -Wextra -Werror -DSERVO_BENCH_MODE=0 -I tests/port_stubs -I Core/Inc -I USB_Log tests/test_motor_pwm.c Core/Src/rt1064_pid_reference.c -lm -o tests/.build/test_motor_pwm.exe
    if ($LASTEXITCODE) { throw 'Motor PWM build failed' }
    & tests/.build/test_motor_pwm.exe
    if ($LASTEXITCODE) { throw 'Motor PWM tests failed' }
    & $Compiler -std=c11 -Wall -Wextra -Werror -DSERVO_BENCH_MODE=1 -I tests/port_stubs -I Core/Inc -I USB_Log tests/test_servo_control.c Core/Src/rt1064_reference.c Core/Src/rt1064_pid_reference.c -lm -o tests/.build/test_servo_control.exe
    if ($LASTEXITCODE) { throw 'Servo build failed' }
    & tests/.build/test_servo_control.exe
    if ($LASTEXITCODE) { throw 'Servo tests failed' }
    & $Compiler -std=c11 -Wall -Wextra -Werror -DSERVO_BENCH_MODE=1 -I tests/port_stubs -I Core/Inc -I USB_Log tests/test_motor_disabled.c Core/Src/rt1064_pid_reference.c -lm -o tests/.build/test_motor_disabled.exe
    if ($LASTEXITCODE) { throw 'Motor exclusion build failed' }
    & tests/.build/test_motor_disabled.exe
    if ($LASTEXITCODE) { throw 'Motor exclusion test failed' }
    & $Compiler -std=c11 -Wall -Wextra -Werror -I tests/port_stubs -I Core/Inc -I USB_Log tests/test_actuators_combined.c Core/Src/rt1064_reference.c Core/Src/rt1064_pid_reference.c -lm -o tests/.build/test_actuators_combined.exe
    if ($LASTEXITCODE) { throw 'Combined actuator build failed' }
    & tests/.build/test_actuators_combined.exe
    if ($LASTEXITCODE) { throw 'Combined actuator tests failed' }
    & $Compiler -std=c11 -Wall -Wextra -Werror -I tests/beep_stubs -I Core/Inc -I hardware/BEEP tests/test_control_tone.c -o tests/.build/test_control_tone.exe
    if ($LASTEXITCODE) { throw 'Control tone build failed' }
    & tests/.build/test_control_tone.exe
    if ($LASTEXITCODE) { throw 'Control tone tests failed' }
    & $Compiler -std=c11 -Wall -Wextra -Werror -I Core/Inc tests/test_power_control.c -lm -o tests/.build/test_power_control.exe
    if ($LASTEXITCODE) { throw 'Power control build failed' }
    & tests/.build/test_power_control.exe
    if ($LASTEXITCODE) { throw 'Power control tests failed' }
    & $Python -c "from pathlib import Path; s=Path('Core/Src/main.c').read_text(encoding='utf-8'); Path('tests/.build/power_service.inc').write_text(s[s.index('static PowerControl power_control;'):s.index('void LCD_ServiceHook(void)')],encoding='utf-8')"
    if ($LASTEXITCODE) { throw 'Power service extraction failed' }
    & $Compiler -std=c11 -Wall -Wextra -Werror -I Core/Inc tests/test_power_service.c -lm -o tests/.build/test_power_service.exe
    if ($LASTEXITCODE) { throw 'Power service build failed' }
    & tests/.build/test_power_service.exe
    if ($LASTEXITCODE) { throw 'Power service tests failed' }
    & $Compiler -std=c11 -Wall -Wextra -Werror -I Core/Inc tests/test_rt1064_reference.c Core/Src/rt1064_reference.c Core/Src/rt1064_pid_reference.c -lm -o tests/.build/test_rt1064_reference.exe
    if ($LASTEXITCODE) { throw 'RT1064 reference build failed' }
    & tests/.build/test_rt1064_reference.exe
    if ($LASTEXITCODE) { throw 'RT1064 reference tests failed' }
    $previousExe = $env:CAR_PROTOCOL_TEST_EXE
    try {
        $env:CAR_PROTOCOL_TEST_EXE = (Resolve-Path tests/.build/test_car_protocol.exe).Path
        & $Python -m unittest discover -s tests -p test_host_control.py -v
        if ($LASTEXITCODE) { throw 'Host cross-language tests failed' }
        & $Python -m unittest discover -s tests -p test_gamepad.py -v
        if ($LASTEXITCODE) { throw 'Gamepad tests failed' }
        & $Python -m unittest discover -s tests -p test_racing.py -v
        if ($LASTEXITCODE) { throw 'Racing tests failed' }
        & $Python -m unittest discover -s tests -p test_gamepad_service.py -v
        if ($LASTEXITCODE) { throw 'Gamepad service tests failed' }
        & $Python -m unittest discover -s tests -p test_servo_calibration.py -v
        if ($LASTEXITCODE) { throw 'Servo calibration host tests failed' }
    } finally { $env:CAR_PROTOCOL_TEST_EXE = $previousExe }
} finally { Pop-Location }
