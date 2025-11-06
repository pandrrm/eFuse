#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> // เพื่อใช้ uint8_t (byte)

// ชื่อไฟล์ที่ใช้จำลองเป็นฮาร์ดแวร์
const char* FUSE_FILE = "efuse_simulator.bin";
// ขนาดของแถวฟิวส์ (จำลองว่ามี 256 ฟิวส์)
#define FUSE_BANK_SIZE 256

/*
 * ฟังก์ชันเริ่มต้น: สร้างไฟล์ฟิวส์ถ้ายังไม่มี
 * (เหมือนชิปตัวใหม่จากโรงงาน)
 */
void initialize_fuses() {
    FILE* file = fopen(FUSE_FILE, "rb"); // ลองเปิดอ่าน
    if (file == NULL) {
        // ถ้าไฟล์ไม่มี (เปิดไม่ออก) ให้สร้างใหม่
        file = fopen(FUSE_FILE, "wb"); // สร้างใหม่แบบเขียน
        if (file == NULL) {
            printf("ERROR: ไม่สามารถสร้างไฟล์จำลองฟิวส์ได้!\n");
            exit(1);
        }

        // เขียนค่า 0x00 (ยังไม่เผา) ทั้งหมด 256 bytes
        uint8_t unblown_fuse = 0x00;
        for (int i = 0; i < FUSE_BANK_SIZE; i++) {
            fwrite(&unblown_fuse, sizeof(uint8_t), 1, file);
        }
        fclose(file);
        printf("...สร้างชิปใหม่ (efuse_loader.bin) เรียบร้อยแล้ว (ฟิวส์ทั้งหมด = 0x00)\n");
    } else {
        // ถ้าไฟล์มีอยู่แล้ว ก็ไม่ต้องทำอะไร
        fclose(file);
        printf("...ตรวจพบ eFuse! (efuse_loader.bin) ที่มีอยู่เดิม\n");
    }
}

/*
 * ฟังก์ชันอ่านค่าฟิวส์
 * (การอ่านทำได้เสมอ)
 */
uint8_t read_fuse(int index) {
    if (index < 0 || index >= FUSE_BANK_SIZE) {
        printf("ERROR: ตำแหน่งฟิวส์ %d ไม่ถูกต้อง!\n", index);
        return 0xFF; // คืนค่า error
    }

    FILE* file = fopen(FUSE_FILE, "rb");
    if (file == NULL) {
        printf("ERROR: ไม่พบไฟล์จำลองฟิวส์!\n");
        return 0xFF; // คืนค่า error
    }

    // เลื่อนไปยังตำแหน่ง (index) ที่ต้องการอ่าน
    fseek(file, index, SEEK_SET);

    uint8_t value = 0x00;
    fread(&value, sizeof(uint8_t), 1, file);
    fclose(file);

    return value;
}

/*
 * ฟังก์ชันเผาฟิวส์ (หัวใจสำคัญ)
 */
int burn_fuse(int index, uint8_t value_to_burn) {
    if (index < 0 || index >= FUSE_BANK_SIZE) {
        printf("ERROR: ตำแหน่งฟิวส์ %d ไม่ถูกต้อง!\n", index);
        return -1; // Error
    }
    
    if (value_to_burn == 0x00) {
        printf("LOGIC ERROR: ไม่สามารถเผาค่า 0x00 ได้ (เพราะ 0x00 หมายถึงยังไม่เผา)\n");
        return -1; // Error
    }

    // 1. อ่านค่าปัจจุบันก่อน
    uint8_t current_value = read_fuse(index);

    // 2. ตรวจสอบกฎการเผา
    if (current_value != 0x00) {
        // ถ้าฟิวส์ไม่ใช่ 0x00 แสดงว่ามันถูกเผาไปแล้ว
        printf("!!! FAILED TO BURN FUSE #%d !!!\n", index);
        printf("    -> เหตุผล: ฟิวส์นี้ถูกเผาไปแล้ว (มีค่า 0x%02X), ไม่สามารถเขียนทับได้!\n", current_value);
        return 0; // ล้มเหลว (แต่ไม่ใช่ Error)
    }

    // 3. ถ้ามาถึงตรงนี้ได้ แสดงว่าฟิวส์ยังว่าง (0x00) -> ทำการเผา
    printf("...กำลังเผาฟิวส์ #%d ด้วยค่า 0x%02X ...\n", index, value_to_burn);
    
    // "r+b" คือโหมดอ่านและเขียนทับ
    FILE* file = fopen(FUSE_FILE, "r+b"); 
    if (file == NULL) {
        printf("ERROR: ไม่สามารถเปิดไฟล์เพื่อเผาได้!\n");
        return -1; // Error
    }

    // เลื่อนไปยังตำแหน่งที่จะเผา
    fseek(file, index, SEEK_SET);
    // "เผา" (เขียนค่าใหม่ลงไป)
    fwrite(&value_to_burn, sizeof(uint8_t), 1, file);
    fclose(file);

    printf("    -> SUCCESS: เผาฟิวส์ #%d สำเร็จ!\n", index);
    return 1; // สำเร็จ
}

// ------------------------------------
// -- โปรแกรมหลักสำหรับทดสอบ --
// ------------------------------------
int main() {
    // 1. เตรียมชิป(สร้างไฟล์ถ้ายังไม่มี)
    initialize_fuses();

    printf("\n--- วาระ 1: พยายามเผาฟิวส์ #5 (Anti-Rollback Version) ---\n");
    // ลองเผาฟิวส์หมายเลข 5 ให้มีค่าเป็น 0x01 (สมมติว่าเป็นเวอร์ชัน 1)
    burn_fuse(5, 0x01);
    printf("    -> ค่าปัจจุบันของฟิวส์ #5: 0x%02X\n", read_fuse(5));

    printf("\n--- วาระ2 2: พยายามเผาฟิวส์ #5 ซ้ำ (ดาวน์เกรด/เขียนทับ) ---\n");
    // ลองเผาฟิวส์หมายเลข 5 ซ้ำด้วยค่า 0x02
    burn_fuse(5, 0x02);
    printf("    -> ค่าปัจจุบันของฟิวส์ #5: 0x%02X\n", read_fuse(5));
    
    printf("\n--- วาระ3 3: พยายามเผาฟิวส์ #10 (Serial Number) ---\n");
    // ลองเผาฟิวส์ตัวอื่น (เบอร์ 10)
    burn_fuse(10, 0xAB);
    printf("    -> ค่าปัจจุบันของฟิวส์ #10: 0x%02X\n", read_fuse(10));
    
    printf("\nการทดสอบเสร็จสิ้น...(Loader Close Bay!)\n");

    return 0;
}