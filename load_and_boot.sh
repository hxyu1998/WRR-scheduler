if [ -d "build-master" ]; then
	# Control will enter here if $DIRECTORY doesn't exist.
	cd kernel
	./make_kernel -j10
	cd ..
	tools/make_bootimg
	adb reboot bootloader
	fastboot devices
	fastboot boot build-master/boot.img
fi
