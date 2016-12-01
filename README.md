<h1> Kenel Tree for CoolPad Note 3 Lite(lollipop)</h1>

<h3>How to Compile this kernel :</h3>	

	PATH=${PATH}:~/aarch64-linux-android-4.9/bin/
	export ARCH=arm64
	make CP8298_I00_defconfig ARCH=arm64 CROSS_COMPILE=aarch64-linux-android-
	make ARCH=arm64 CROSS_COMPILE=aarch64-linux-android-
	
<h3>Special Thanks to :</h3>
	
	Pinnamanivenkat(me)
	Linus Torvalds( For the kernel initiative)
	CoolPad company( For such a wonderful source)
	Team AMT
