mkdir -p iso
sudo losetup -P /dev/loop0 b.img
sudo mount /dev/loop0p1 iso/
