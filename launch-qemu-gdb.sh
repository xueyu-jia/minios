# run qemu, with gdb support
# added by xw, 18/6

# deleted by mingxuan 2019-5-17
# gnome-terminal -x bash -c "echo 'type in gdb: target remote :1234';echo '';gdb -s kernel.gdb.bin" &
# qemu-system-i386 -fda a.img -hda 80m.img -boot order=a -ctrl-grab \
# -gdb tcp::1234 -S -monitor stdio
# qemu-system-i386 -m 2048 -hda b.img -boot order=a -ctrl-grab \
# -gdb tcp::1234 -S  -monitor stdio
# modified by mingxuan 2021-8-8

# gnome-terminal -x bash -c "echo 'type in gdb: target remote :1234';echo '';gdb -s kernel.gdb.bin" &
# qemu-system-i386 -drive id=disk,file=b.img,if=none -device ich9-ahci,id=ahci \
# -device ide-hd,drive=disk,bus=ahci.0 -gdb tcp::1234 -S -monitor stdio

#多个SATA设备 1个IDE设备
# qemu-system-i386 \
# -drive id=disk,file=b.img,if=none -device ich9-ahci,id=ahci -device ide-hd,drive=disk,bus=ahci.0 \
# -drive id=disk1,file=c.img,if=none -device ich9-ahci,id=ahci1 -device ide-hd,drive=disk1,bus=ahci.1 \
# -drive id=disk2,file=d.img,if=none -device ich9-ahci,id=ahci2 -device ide-hd,drive=disk2,bus=ahci.2 \
# -hdb a.img -gdb tcp::1234 -S -monitor stdio

#多个SATA设备 2个IDE设备
cp b.img a.img
# cp b.img c.img
cp b.img d.img
cp b.img e.img

qemu-system-i386 \
-drive id=disk,file=b.img,if=none -device ich9-ahci,id=ahci -device ide-hd,drive=disk,bus=ahci.0 \
-drive id=disk1,file=c.img,if=none -device ich9-ahci,id=ahci1 -device ide-hd,drive=disk1,bus=ahci.1 \
-drive id=disk2,file=d.img,if=none -device ich9-ahci,id=ahci2 -device ide-hd,drive=disk2,bus=ahci.2 \
-hda a.img -hdb e.img -gdb tcp::1234 -S -monitor stdio