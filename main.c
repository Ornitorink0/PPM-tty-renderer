#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdint.h>

// Struttura per rappresentare un pixel
typedef struct
{
    unsigned char r, g, b;
} Pixel;

/**
 * @brief Esegue il rendering di un'immagine PPM su un framebuffer
 *
 * La funzione apre il file dell'immagine in formato PPM, legge l'intestazione
 * e i dati dell'immagine, li copia in un array di struct Pixel e li stampa
 * su un framebuffer.
 *
 * @returns 0 se l'operazione va a buon fine, 1 in caso di errore
 */
int main()
{
    // Apriamo il file dell'immagine in formato PPM
    FILE *image = fopen("post.ppm", "rb");
    if (image == NULL)
    {
        // Se non riusciamo ad aprire il file, stampiamo un messaggio di errore
        perror("Errore apertura immagine");
        return 1;
    }

    // Leggiamo l'intestazione del file PPM
    char format[3];
    int width, height, maxval;

    fscanf(image, "%s\n%d %d\n%d\n", format, &width, &height, &maxval);
    printf("Formato: %s\n", format);
    printf("Larghezza: %d\n", width);
    printf("Altezza: %d\n", height);
    printf("Valore massimo: %d\n", maxval);

    // Allocchiamo memoria per l'immagine
    Pixel *renderedImage = malloc(width * height * sizeof(Pixel));
    if (!renderedImage)
    {
        // Se non riusciamo ad allocare la memoria, stampiamo un messaggio di errore
        printf("Errore allocazione memoria\n");
        fclose(image);
        return 1;
    }

    // Leggiamo i dati dell'immagine e li copiamo in un array di struct Pixel
    for (size_t i = 0; i < width * height; i++)
    {
        int r, g, b;
        fscanf(image, "%d %d %d\n", &r, &g, &b);
        renderedImage[i].r = r;
        renderedImage[i].g = g;
        renderedImage[i].b = b;
        // printf("Pixel %d: R=%d, G=%d, B=%d\n", i, r, g, b);
    }

    // Chiudiamo il file dell'immagine
    fclose(image);

    // Apriamo il file di dispositivo del framebuffer
    int fb = open("/dev/fb0", O_RDWR);
    if (fb == -1)
    {
        // Se non riusciamo ad aprire il file, stampiamo un messaggio di errore
        perror("Errore apertura /dev/fb0");
        return 1;
    }

    // Recuperiamo informazioni sul framebuffer
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;

    if (ioctl(fb, FBIOGET_FSCREENINFO, &finfo) == -1 ||
        ioctl(fb, FBIOGET_VSCREENINFO, &vinfo) == -1)
    {
        // Se non riusciamo a recuperare le informazioni, stampiamo un messaggio di errore
        perror("Errore ioctl");
        close(fb);
        return 1;
    }

    // Calcoliamo la dimensione della memoria video
    long screensize = vinfo.yres_virtual * finfo.line_length;

    // Mappiamo la memoria video in memoria virtuale
    uint8_t *fb_ptr = (uint8_t *)mmap(0, screensize, PROT_READ | PROT_WRITE, MAP_SHARED, fb, 0);
    if (fb_ptr == MAP_FAILED)
    {
        // Se non riusciamo a mappare la memoria, stampiamo un messaggio di errore
        perror("Errore mmap");
        close(fb);
        return 1;
    }

    // Calcoliamo il numero di byte per pixel
    int bytes_per_pixel = vinfo.bits_per_pixel / 8;

    int max_x = width < vinfo.xres ? width : vinfo.xres;
    int max_y = height < vinfo.yres ? height : vinfo.yres;

    // Copiamo l'immagine nella memoria video
    for (int y = 0; y < max_y; y++)
    {
        for (int x = 0; x < max_x; x++)
        {
            int offset = y * finfo.line_length + x * bytes_per_pixel;
            Pixel p = renderedImage[y * width + x];

            if (vinfo.bits_per_pixel == 32)
            {
                fb_ptr[offset] = p.b;     // Blue
                fb_ptr[offset + 1] = p.g; // Green
                fb_ptr[offset + 2] = p.r; // Red
                fb_ptr[offset + 3] = 0;   // Alpha o padding
            }
            else if (vinfo.bits_per_pixel == 24)
            {
                fb_ptr[offset] = p.b;
                fb_ptr[offset + 1] = p.g;
                fb_ptr[offset + 2] = p.r;
            }
            else
            {
                fprintf(stderr, "Formato video non supportato (%d bpp)\n", vinfo.bits_per_pixel);
                munmap(fb_ptr, screensize);
                close(fb);
                free(renderedImage);
                return 1;
            }
        }
    }

    // Svincoliamo la memoria video
    munmap(fb_ptr, screensize);
    // Chiudiamo il file di dispositivo del framebuffer
    close(fb);
    return 0;
}
