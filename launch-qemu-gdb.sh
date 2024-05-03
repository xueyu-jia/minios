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


# USB 启动
# qemu-system-i386 \
# -drive if=none,id=usbstick,format=raw,file=b.img    \
# -device nec-usb-xhci,id=ehci                            \
# -device usb-storage,bus=ehci.0,drive=usbstick       \
# -boot menu=on \
# -gdb tcp::1234 -S -monitor stdio

#多个SATA设备 1个IDE设备
# qemu-system-i386 \
# -drive id=disk,file=b.img,if=none -device ich9-ahci,id=ahci -device ide-hd,drive=disk,bus=ahci.0 \
# -drive id=disk1,file=c.img,if=none -device ich9-ahci,id=ahci1 -device ide-hd,drive=disk1,bus=ahci.1 \
# -drive id=disk2,file=d.img,if=none -device ich9-ahci,id=ahci2 -device ide-hd,drive=disk2,bus=ahci.2 \
# -hdb a.img -gdb tcp::1234 -S -monitor stdio

#多个SATA设备 2个IDE设备
# cp b.img a.img
# cp b.img c.img
# cp b.img d.img
# cp b.img e.img
# cp b.img f.img

# qemu-system-x86_64 \
# -device ich9-ahci,id=xiaofeng \
# -drive id=disk,file=b.img,if=none -device ide-hd,drive=disk,bus=xiaofeng.0 \
# -drive id=disk1,file=j.img,if=none -device ide-hd,drive=disk1,bus=xiaofeng.1  \
# -drive id=disk2,file=d.img,if=none -device ide-hd,drive=disk2,bus=xiaofeng.2  \
# -drive id=disk3,file=f.img,if=none -device ide-hd,drive=disk3,bus=xiaofeng.3  \
# -hda a.img -hdb e.img \
# -boot menu=on \
#  -gdb tcp::1234 -S -monitor stdio
desktop_env=$XDG_CURRENT_DESKTOP
if [[ $desktop_env == "KDE" ]];then
	terminal="konsole -e"
else 
	terminal="gnome-terminal -x"
fi
# $terminal bash -c "echo 'type in gdb: target remote :1234';echo '';gdb -s kernel.gdb.bin" &
qemu-system-i386 \
-device ich9-ahci,id=xiaofeng \
-drive id=disk,file=b.img,if=none -device ide-hd,drive=disk,bus=xiaofeng.0 \
-rtc base=utc \
-boot menu=on \
-gdb tcp::1234 -S -monitor stdio
