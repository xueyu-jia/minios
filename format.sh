sudo losetup -P /dev/loop1 ./b.img
sudo ./format /dev/loop1p5 
sudo losetup -d /dev/loop1
