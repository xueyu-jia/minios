if [ -d iso ]; then
    sudo umount iso/
    sudo losetup -d /dev/loop0
fi
