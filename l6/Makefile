LINUX_VER=5.4.43
KERN_DIR=kernel/
HOSTNAME=andrzejek-pici

.PHONY: default build kernel rootfs run clean

default:
	$(MAKE) -C . build

# Compiles QEMU-bootable kernel.
build:
	cp -f src/syscalls.h $(KERN_DIR)include/linux/
	cp -f src/syscall_64.tbl $(KERN_DIR)arch/x86/entry/syscalls/
	cp -f src/Makefile $(KERN_DIR)Makefile
	cp -rf src/mysyscalls $(KERN_DIR)
	$(MAKE) -C $(KERN_DIR) defconfig
	$(MAKE) -C $(KERN_DIR) kvmconfig
	sed -i -E 's/(CONFIG_DEFAULT_HOSTNAME)=.*/\1="$(HOSTNAME)"/g' $(KERN_DIR).config
	$(MAKE) -C $(KERN_DIR) -j8

# Downloads and extracts Linux kernel.
kernel:
	wget -O kernel.tar.xz https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-$(LINUX_VER).tar.xz
	tar -xvf kernel.tar.xz
	mv linux-$(LINUX_VER) $(KERN_DIR)
	rm *.tar.*

# Prepares Alpine Linux based rootfs.
rootfs:
	gcc -static src/test.c -o ./test
	sudo ./mk_rootfs.sh \
		--chroot "echo -e 'user\nuser' | busybox adduser -s /bin/ash user" \
		--after "mv test /mnt/home/user/test"

# Start QEMU emulator
run:
	qemu-system-x86_64 \
		-m 1024 \
		-kernel $(KERN_DIR)arch/x86_64/boot/bzImage \
		-drive format=raw,media=disk,file=rootfs.img \
		-append "root=/dev/sda"

clean:
	rm -rf *.tar.* *.img $(KERN_DIR) linux-* rootfs
