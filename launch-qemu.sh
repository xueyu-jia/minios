# run qemu, without using gdb
# added by xw, 18/6

# deleted by mingxuan 2019-5-17
# qemu-system-i386 -fda a.img -hda 80m.img -boot order=a -ctrl-grab \
# -monitor stdio

# modified by mingxuan 2019-5-17
# qemu-system-i386 -m 2048 -hda b.img -boot order=a -ctrl-grab \
# -monitor stdio -usbdevice tablet

# qemu-system-i386 -drive id=disk,file=b.img,if=none -device ich9-ahci,id=ahci \
# -device ide-hd,drive=disk,bus=ahci.0 -monitor stdio
# cp b.img a.img
# cp b.img c.img
# cp b.img d.img
# cp b.img e.img

# qemu-system-i386 \
# -drive id=disk,file=b.img,if=none -device ich9-ahci,id=ahci -device ide-hd,drive=disk,bus=ahci.0 \
# -drive id=disk1,file=c.img,if=none -device ide-hd,drive=disk1,bus=ahci.1 \
# -drive id=disk2,file=d.img,if=none -device ide-hd,drive=disk2,bus=ahci.2 \
# -hda a.img -hdb e.img
cp b.img c.img
qemu-system-i386 \
-device ich9-ahci,id=xiaofeng \
-drive id=disk,file=a.img,if=none -device ide-hd,drive=disk,bus=xiaofeng.0 \
-hda b.img -hdb c.img \
-boot menu=on \
-monitor stdio