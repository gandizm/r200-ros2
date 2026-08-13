// SPDX-License-Identifier: Apache-2.0
#include <errno.h>
#include <fcntl.h>
#include <linux/uvcvideo.h>
#include <linux/videodev2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <unistd.h>

static int xu(int fd, unsigned char query, unsigned char unit, unsigned char selector,
              void *data, unsigned short size, const char *label)
{
    struct uvc_xu_control_query q = {unit, selector, query, size, (unsigned char *)data};
    errno = 0;
    int rc = ioctl(fd, UVCIOC_CTRL_QUERY, &q);
    printf("%s: rc=%d errno=%d (%s)\n", label, rc, errno, strerror(errno));
    return rc;
}

int main(int argc, char **argv)
{
    const char *node = argc > 1 ? argv[1] : "/dev/video0";
    int do_sfmt = argc > 2 ? atoi(argv[2]) : 0;
    int do_stream = argc > 4 ? atoi(argv[4]) : 0;
    int fd = open(node, O_RDWR | O_NONBLOCK, 0);
    if (fd < 0) { perror("open"); return 1; }

    if (do_sfmt)
    {
        struct v4l2_format fmt;
        memset(&fmt, 0, sizeof(fmt));
        fmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        fmt.fmt.pix.width = 640;
        fmt.fmt.pix.height = 481;
        fmt.fmt.pix.pixelformat = v4l2_fourcc('Y', '1', '2', 'I');
        fmt.fmt.pix.field = V4L2_FIELD_NONE;
        errno = 0;
        int rc = ioctl(fd, VIDIOC_S_FMT, &fmt);
        printf("S_FMT Y12I 640x481: rc=%d errno=%d (%s)\n", rc, errno, strerror(errno));

        /* open and configure the metadata node of the same interface (RS2 order) */
        const char *meta = argc > 3 ? argv[3] : "/dev/video1";
        int mfd = open(meta, O_RDWR | O_NONBLOCK, 0);
        if (mfd >= 0)
        {
            struct v4l2_format mfmt;
            memset(&mfmt, 0, sizeof(mfmt));
            mfmt.type = V4L2_BUF_TYPE_META_CAPTURE;
            mfmt.fmt.meta.dataformat = v4l2_fourcc('D', '4', 'X', 'X');
            errno = 0;
            int mrc = ioctl(mfd, VIDIOC_S_FMT, &mfmt);
            printf("meta %s S_FMT D4XX: rc=%d errno=%d (%s)\n", meta, mrc, errno, strerror(errno));
        }
        else
        {
            printf("meta %s open failed: %s\n", meta, strerror(errno));
        }

        /* fps */
        struct v4l2_streamparm parm;
        memset(&parm, 0, sizeof(parm));
        parm.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        ioctl(fd, VIDIOC_G_PARM, &parm);
        parm.parm.capture.timeperframe.numerator = 1;
        parm.parm.capture.timeperframe.denominator = 30;
        errno = 0;
        int prc = ioctl(fd, VIDIOC_S_PARM, &parm);
        printf("S_PARM 30fps: rc=%d errno=%d (%s)\n", prc, errno, strerror(errno));

        /* request and queue one buffer, like the backend does */
        struct v4l2_requestbuffers req;
        memset(&req, 0, sizeof(req));
        req.count = 4;
        req.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        req.memory = V4L2_MEMORY_MMAP;
        errno = 0;
        int rrc = ioctl(fd, VIDIOC_REQBUFS, &req);
        printf("REQBUFS 4: rc=%d errno=%d (%s)\n", rrc, errno, strerror(errno));

        struct v4l2_buffer buf;
        memset(&buf, 0, sizeof(buf));
        buf.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        buf.memory = V4L2_MEMORY_MMAP;
        buf.index = 0;
        errno = 0;
        int qrc = ioctl(fd, VIDIOC_QUERYBUF, &buf);
        printf("QUERYBUF 0: rc=%d errno=%d (%s)\n", qrc, errno, strerror(errno));
        if (qrc == 0)
        {
            void *p = mmap(NULL, buf.length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, buf.m.offset);
            printf("mmap: %p\n", p);
            errno = 0;
            int bqc = ioctl(fd, VIDIOC_QBUF, &buf);
            printf("QBUF 0: rc=%d errno=%d (%s)\n", bqc, errno, strerror(errno));
        }

        if (mfd >= 0)
            close(mfd);

        /* configure the other two R200 interfaces like RS2 does */
        struct { const char *node; const char *meta; unsigned int fourcc; int w, h; } others[2] = {
            {"/dev/video4", "/dev/video5", 0, 628, 469},   /* Z16 depth */
            {"/dev/video6", "/dev/video7", 0, 640, 480},   /* YUYV color */
        };
        others[0].fourcc = v4l2_fourcc('Z', '1', '6', ' ');
        others[1].fourcc = v4l2_fourcc('Y', 'U', 'Y', 'V');
        for (int k = 0; k < 2; ++k)
        {
            int ofd = open(others[k].node, O_RDWR | O_NONBLOCK, 0);
            if (ofd < 0) { printf("open %s failed: %s\n", others[k].node, strerror(errno)); continue; }
            struct v4l2_format ofmt;
            memset(&ofmt, 0, sizeof(ofmt));
            ofmt.type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
            ofmt.fmt.pix.width = others[k].w;
            ofmt.fmt.pix.height = others[k].h;
            ofmt.fmt.pix.pixelformat = others[k].fourcc;
            ofmt.fmt.pix.field = V4L2_FIELD_NONE;
            errno = 0;
            int orc = ioctl(ofd, VIDIOC_S_FMT, &ofmt);
            printf("%s S_FMT %dx%d: rc=%d errno=%d (%s)\n", others[k].node,
                   others[k].w, others[k].h, orc, errno, strerror(errno));

            int omfd = open(others[k].meta, O_RDWR | O_NONBLOCK, 0);
            if (omfd >= 0)
            {
                struct v4l2_format mfmt2;
                memset(&mfmt2, 0, sizeof(mfmt2));
                mfmt2.type = V4L2_BUF_TYPE_META_CAPTURE;
                mfmt2.fmt.meta.dataformat = v4l2_fourcc('D', '4', 'X', 'X');
                errno = 0;
                int omrc = ioctl(omfd, VIDIOC_S_FMT, &mfmt2);
                printf("%s meta S_FMT: rc=%d errno=%d (%s)\n", others[k].meta, omrc, errno, strerror(errno));
                close(omfd);
            }
            close(ofd);
        }
    }

    if (do_stream)
    {
        enum v4l2_buf_type type = V4L2_BUF_TYPE_VIDEO_CAPTURE;
        errno = 0;
        int src = ioctl(fd, VIDIOC_STREAMON, &type);
        printf("STREAMON fd1: rc=%d errno=%d (%s)\n", src, errno, strerror(errno));

        /* open a second fd and run the XU probes below against it */
        int fd2 = open(node, O_RDWR | O_NONBLOCK, 0);
        if (fd2 < 0) { perror("open fd2"); return 1; }
        close(fd);
        fd = fd2;
    }

    unsigned char intent = 0x07;
    xu(fd, 0x01 /*UVC_SET_CUR*/, 2, 3, &intent, sizeof(intent), "SET stream_intent(3)");

    /* DS4 CommandResponsePacket: get_fwrevision (0x21), direct (0x10), tag 12 */
    unsigned char cmd[256] = {0};
    cmd[0] = 0x21; /* command code, LE */
    cmd[4] = 0x10; /* modifier, LE */
    cmd[8] = 12;   /* tag, LE */
    xu(fd, 0x01, 2, 1, cmd, sizeof(cmd), "SET command_response(1) get_fwrevision");
    xu(fd, 0x81 /*UVC_GET_CUR*/, 2, 1, cmd, sizeof(cmd), "GET command_response(1)");

    unsigned char depth_units[4] = {0};
    xu(fd, 0x81, 2, 4, depth_units, sizeof(depth_units), "GET depth_units(4)");

    close(fd);
    return 0;
}
