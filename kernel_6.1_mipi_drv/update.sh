#!/bin/sh
cp Kconfig /Devel/alcor/alcor-2023.08.4/output/build/linux-lf-6.6.y/drivers/gpu/drm/panel/.
cp Makefile /Devel/alcor/alcor-2023.08.4/output/build/linux-lf-6.6.y/drivers/gpu/drm/panel/.
cp panel-tricomtek_ips1024600.c /Devel/alcor/alcor-2023.08.4/output/build/linux-lf-6.6.y/drivers/gpu/drm/panel/.
rm /Devel/alcor/alcor-2023.08.4/output/build/linux-lf-6.6.y/.stamp_configured
rm /Devel/alcor/alcor-2023.08.4/output/build/linux-lf-6.6.y/.stamp_built
