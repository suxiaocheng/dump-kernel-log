# dump-kernel-log

## Setting in the kernel cmdline
```
commit 07bf184296fda1cd57ba4adedc5c858611dd32fe
Author: suxiaocheng <Xiaocheng.Su@desay-svautomotive.com>
Date:   Wed Mar 25 10:53:09 2020 +0800

    arm64: dts: g6sh: reserved 64KB for pstore to dump kernel log

diff --git a/arch/arm64/boot/dts/renesas/qnxhyp-android-m3-g6sh.dts b/arch/arm64/boot/dts/renesas/qnxhyp-android-m3-g6sh.dts
index ea4a135..68c4476 100755
--- a/arch/arm64/boot/dts/renesas/qnxhyp-android-m3-g6sh.dts
+++ b/arch/arm64/boot/dts/renesas/qnxhyp-android-m3-g6sh.dts
@@ -59,6 +59,14 @@
                        compatible = "ion-region";
                        reg = <0x00000000 0x6c000000 0x0 0x14000000>; /* 320Mb */
                };
+
+               /* pstore: reserved memory to save console msg */
+               ramoops: ramoops@6bff0000{
+                       compatible = "ramoops";
+                       reg = <0x0 0x6bff0000 0x0 0x10000>;
+                       record-size = <0x4000>;                 /* 64KB */
+                       console-size = <0x10000>;
+               };
        };

        ion {


```

## dump in the qnx side

` dump-kernel-log -o /var/dmesg.txt `
