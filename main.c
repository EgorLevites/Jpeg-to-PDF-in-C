#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include <jpeglib.h>
#include <hpdf.h>

// Function to check if a file is a JPEG
int is_jpeg(const char *filename) {
    FILE *file = fopen(filename, "rb");
    if (!file) return 0;
    
    unsigned char buffer[2];
    fread(buffer, 1, 2, file);
    fclose(file);
    
    return buffer[0] == 0xFF && buffer[1] == 0xD8;
}

// Function to convert JPEG images to PDF and move the JPEGs to another folder
void convert_and_move_jpg_to_pdf(const char *source_folder, const char *moved_jpg_folder, const char *log_file) {
    struct stat st = {0};

    // Create moved JPEG folder if it doesn't exist
    if (stat(moved_jpg_folder, &st) == -1) {
        mkdir(moved_jpg_folder, 0700);
    }

    DIR *dir = opendir(source_folder);
    if (dir == NULL) {
        printf("Source folder %s does not exist.\n", source_folder);
        return;
    }

    struct dirent *entry;
    FILE *log_fp = fopen(log_file, "a");
    if (log_fp == NULL) {
        printf("Error opening log file %s.\n", log_file);
        closedir(dir);
        return;
    }

    // Loop through each file in the source folder
    while ((entry = readdir(dir)) != NULL) {
        if (strstr(entry->d_name, ".jpg") || strstr(entry->d_name, ".jpeg")) {
            char image_path[512];
            snprintf(image_path, sizeof(image_path), "%s/%s", source_folder, entry->d_name);

            // Check if the file is a valid JPEG
            if (!is_jpeg(image_path)) {
                printf("Not a JPEG file: %s\n", image_path);
                continue;
            }

            // Create a new PDF document
            HPDF_Doc pdf = HPDF_New(NULL, NULL);
            if (!pdf) {
                printf("Error creating PDF object.\n");
                continue;
            }

            HPDF_Page page = HPDF_AddPage(pdf);
            HPDF_Page_SetWidth(page, 595);
            HPDF_Page_SetHeight(page, 842);

            // Read the JPEG file
            struct jpeg_decompress_struct cinfo;
            struct jpeg_error_mgr jerr;
            FILE *jpg_fp = fopen(image_path, "rb");
            if (!jpg_fp) {
                printf("Error opening image %s.\n", image_path);
                HPDF_Free(pdf);
                continue;
            }

            cinfo.err = jpeg_std_error(&jerr);
            jpeg_create_decompress(&cinfo);
            jpeg_stdio_src(&cinfo, jpg_fp);
            jpeg_read_header(&cinfo, TRUE);
            jpeg_start_decompress(&cinfo);

            // Allocate memory for the JPEG image
            unsigned char *buffer = (unsigned char *)malloc(cinfo.output_width * cinfo.output_height * cinfo.output_components);
            while (cinfo.output_scanline < cinfo.output_height) {
                unsigned char *row_pointer = buffer + cinfo.output_scanline * cinfo.output_width * cinfo.output_components;
                jpeg_read_scanlines(&cinfo, &row_pointer, 1);
            }

            jpeg_finish_decompress(&cinfo);
            jpeg_destroy_decompress(&cinfo);
            fclose(jpg_fp);

            // Add the JPEG image to the PDF
            HPDF_Image pdf_image = HPDF_LoadRawImageFromMem(pdf, buffer, cinfo.output_width, cinfo.output_height, HPDF_CS_DEVICE_RGB, 8);
            HPDF_Page_DrawImage(page, pdf_image, 0, 0, 595, 842);

            char pdf_path[512];
            snprintf(pdf_path, sizeof(pdf_path), "%s/%s.pdf", source_folder, entry->d_name);
            HPDF_SaveToFile(pdf, pdf_path);
            HPDF_Free(pdf);
            free(buffer);

            printf("Converted %s to %s\n", entry->d_name, pdf_path);

            // Move the JPEG file to the moved JPEG folder
            char moved_jpg_path[512];
            snprintf(moved_jpg_path, sizeof(moved_jpg_path), "%s/%s", moved_jpg_folder, entry->d_name);
            if (rename(image_path, moved_jpg_path) == 0) {
                printf("Moved %s to %s\n", entry->d_name, moved_jpg_path);
            } else {
                printf("Error moving %s to %s\n", entry->d_name, moved_jpg_path);
            }

            // Log the processed file name
            fprintf(log_fp, "%s\n", entry->d_name);
        }
    }

    fclose(log_fp);
    closedir(dir);
}

int main() {
    const char *source_folder = "/path";
    const char *moved_jpg_folder = "/path";
    const char *log_file = "/path to list.txt";

    convert_and_move_jpg_to_pdf(source_folder, moved_jpg_folder, log_file);

    return 0;
}
