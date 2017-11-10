if [ -d "build-debug" ]; then
# if [ -d "build-master" ]; then
	# Control will enter here if $DIRECTORY doesn't exist.
	tools/make_bootimg
	adb reboot bootloader
	fastboot devices
	fastboot boot build-debug/boot.img
	# fastboot boot build-master/boot.img
fi
