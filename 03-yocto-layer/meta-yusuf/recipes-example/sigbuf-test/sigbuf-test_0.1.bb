SUMMARY = "Userspace test suite for the sigbuf character device"
LICENSE = "GPL-2.0-only"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/GPL-2.0-only;md5=801f80980d171dd6425610833a22dbe6"

SRC_URI = "file://sigbuf_test.c \
           file://sigbuf_ioctl.h \
          "

S = "${WORKDIR}"

do_compile() {
    ${CC} ${CFLAGS} ${LDFLAGS} -Wall -Wextra -o sigbuf_test sigbuf_test.c
}

do_install() {
    install -d ${D}${bindir}
    install -m 0755 sigbuf_test ${D}${bindir}/
}
