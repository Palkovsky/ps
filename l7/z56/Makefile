LINUX_VER=5.4.43
KERN_DIR=kernel/
HOSTNAME=andrzejek-pici

.PHONY: default build config kernel rootfs run clean

default:
	$(MAKE) -C . config
	$(MAKE) -C . build

# Compiles QEMU-bootable kernel.
build:
	cp -f src/fault.c $(KERN_DIR)arch/x86/mm/
	$(MAKE) -C $(KERN_DIR) -j8

# Updates kernel config.
config:
	$(MAKE) -C $(KERN_DIR) defconfig
	$(MAKE) -C $(KERN_DIR) kvmconfig
	sed -i -E 's/(CONFIG_DEFAULT_HOSTNAME)=.*/\1="$(HOSTNAME)"/g' $(KERN_DIR).config

# Downloads and extracts Linux kernel.
kernel:
	wget -O kernel.tar.xz https://cdn.kernel.org/pub/linux/kernel/v5.x/linux-$(LINUX_VER).tar.xz
	tar -xvf kernel.tar.xz
	mv linux-$(LINUX_VER) $(KERN_DIR)
	rm *.tar.*

# Prepares Alpine Linux based rootfs.
rootfs:
	$(MAKE) -C test/
	sudo bash $(PWD)/mk_rootfs.sh \
		--chroot "echo -e 'user\nuser' | busybox adduser -s /bin/ash user" \
		--after "mv test/test /mnt/home/user/test"

# Boot up compiled kernel in QEMU
run:
	qemu-system-x86_64 \
		-m 1024 \
		-kernel $(KERN_DIR)arch/x86_64/boot/bzImage \
		-drive format=raw,media=disk,file=rootfs.img \
		-append "root=/dev/sda"

# Burn it down
clean:
	rm -rf *.tar.* *.img $(KERN_DIR) linux-* rootfs
