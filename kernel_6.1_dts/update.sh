#!/bin/sh
cp *.dts /Devel/alcor/alcor-2023.08.4/output/build/linux-lf-6.6.y/arch/arm64/boot/dts/freescale/.
cp *.dtsi /Devel/alcor/alcor-2023.08.4/output/build/linux-lf-6.6.y/arch/arm64/boot/dts/freescale/.
cp Makefile /Devel/alcor/alcor-2023.08.4/output/build/linux-lf-6.6.y/arch/arm64/boot/dts/freescale/.
#cp smsc.c /Devel/alcor/alcor-2023.08.4/output/build/linux-lf-6.6.y/drivers/net/phy/smsc.c
rm /Devel/alcor/alcor-2023.08.4/output/build/linux-lf-6.6.y/.stamp_configured
rm /Devel/alcor/alcor-2023.08.4/output/build/linux-lf-6.6.y/.stamp_built
