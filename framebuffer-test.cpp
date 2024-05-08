#include <linux/fb.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/mman.h>

int main(void)
{
    int fbfd = open("/dev/fb0", O_RDWR);
    if (fbfd == -1)
    {
        perror("Error: cannot open framebuffer device");
        return 1;
    }
    struct fb_var_screeninfo vinfo;
    if (ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo))
    {
        perror("Error reading variable information");
        return 1;
    }
    printf("Resolution: %dx%d\n", vinfo.xres, vinfo.yres);
    printf("Virtual Resolution: %dx%d\n", vinfo.xres_virtual, vinfo.yres_virtual);
    printf("Bits per pixel: %d\n", vinfo.bits_per_pixel);
    printf("Grayscale: %d\n", vinfo.grayscale);
    // https://kevinboone.me/linuxfbc.html
    int fb_width = vinfo.xres;
    int fb_height = vinfo.yres;
    int fb_bpp = vinfo.bits_per_pixel;
    int fb_bytes = fb_bpp / 8;
    int fb_data_size = fb_width * fb_height * fb_bytes;

    char *fbdata = mmap(0, fb_data_size,
                        PROT_READ | PROT_WRITE, MAP_SHARED, fbfd, (off_t)0);
    if (fbdata == MAP_FAILED)
    {
        perror("Error: failed to map framebuffer device to memory");
        return 1;
    }
    // draw a red rectangle
    for (int y = 0; y < fb_height; y++)
    {
        for (int x = 0; x < fb_width; x++)
        {
            long location = (x + vinfo.xoffset) * fb_bytes +
                            (y + vinfo.yoffset) * vinfo.xres * fb_bytes;
            if (vinfo.bits_per_pixel == 32)
            {
                *(fbdata + location) = 0;       // blue
                *(fbdata + location + 1) = 0;   // green
                *(fbdata + location + 2) = 255; // red
                *(fbdata + location + 3) = 0;   // transparency
            }
        }
    }
    munmap(fbdata, fb_data_size);
    close(fbfd);
}