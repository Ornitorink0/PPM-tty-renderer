import sys

def convert_ppm_p6_to_p3(input_filename, output_filename):
    with open(input_filename, 'rb') as f:
        # Leggi intestazione
        magic_number = f.readline().strip()
        if magic_number != b'P6':
            raise ValueError("Il file non è in formato P6")
        
        # Salta eventuali commenti
        def read_non_comment_line(file):
            line = file.readline()
            while line.startswith(b'#'):
                line = file.readline()
            return line

        dimensions_line = read_non_comment_line(f)
        width, height = map(int, dimensions_line.strip().split())

        maxval_line = read_non_comment_line(f)
        maxval = int(maxval_line.strip())

        # Leggi i dati binari
        pixel_data = f.read()

    # Scrivi in formato P3 (ASCII)
    with open(output_filename, 'w') as out:
        out.write("P3\n")
        out.write(f"{width} {height}\n")
        out.write(f"{maxval}\n")

        for i in range(0, len(pixel_data), 3):
            r = pixel_data[i]
            g = pixel_data[i+1]
            b = pixel_data[i+2]
            out.write(f"{r} {g} {b}\n")

    print(f"Conversione completata: {output_filename}")

if __name__ == "__main__":
    if len(sys.argv) != 3:
        print("Uso: python3 convert-p3.py <input.ppm> <output.ppm>")
        sys.exit(1)

    convert_ppm_p6_to_p3(sys.argv[1], sys.argv[2])

