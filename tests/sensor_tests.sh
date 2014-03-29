#9DOF
echo "../shell/send value_request 9dof.gyro"
../shell/send value_request 9dof.gyro
sleep 5
echo "../shell/send value_request 9dof.accel"
../shell/send value_request 9dof.accel
sleep 5
echo "../shell/send value_request 9dof.sats"
../shell/send value_request 9dof.sats
sleep 5
echo "../shell/send value_request 9dof.compass"
../shell/send value_request 9dof.compass
sleep 5
echo "../shell/send value_request 9dof.coords"
../shell/send value_request 9dof.coords
sleep 5
echo "../shell/send value_request 9dof.time"
../shell/send value_request 9dof.time
sleep 5

#DIAG
echo "../shell/send value_request diag.temp5"
../shell/send value_request diag.temp5
sleep 5
echo "../shell/send value_request diag.temp6"
../shell/send value_request diag.temp6
sleep 5
echo "../shell/send value_request diag.temp7"
../shell/send value_request diag.temp7
sleep 5
echo "../shell/send value_request diag.temp8"
../shell/send value_request diag.temp8
sleep 5
echo "../shell/send value_request diag.voltage"
../shell/send value_request diag.voltage
sleep 5
echo "../shell/send value_request diag.current"
../shell/send value_request diag.current
sleep 5

#ENVIRO
echo "../shell/send value_request enviro.temp1"
../shell/send value_request enviro.temp1
sleep 5
echo "../shell/send value_request enviro.temp2"
../shell/send value_request enviro.temp2
sleep 5
echo "../shell/send value_request enviro.rain"
../shell/send value_request enviro.rain
sleep 5
echo "../shell/send value_request enviro.wind"
../shell/send value_request enviro.wind
sleep 5
echo "../shell/send value_request enviro.humidity"
../shell/send value_request enviro.humidity
sleep 5
echo "../shell/send value_request enviro.pressure"
../shell/send value_request enviro.pressure
sleep 5

#ARM
echo "../shell/send value_request arm.arm"
../shell/send value_request arm.arm
sleep 5
echo "../shell/send value_request arm.laser"
../shell/send value_request arm.laser
sleep 10 # extra long
echo "../shell/send value_request arm.stepper"
../shell/send value_request arm.stepper
sleep 5

#MODEMA
echo "../shell/send value_request modema.uptime"
../shell/send value_request modema.uptime
sleep 5

#MODEMB
echo "../shell/send value_request modemb.uptime"
../shell/send value_request modemb.uptime
