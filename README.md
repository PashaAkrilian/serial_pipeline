# serial_pipeline

Pipeline C++ untuk membaca data biner dari port serial, menyusunnya menjadi frame, lalu memprosesnya secara paralel dengan thread pool dan mencatatnya ke file log.

Proyek ini dibuat sebagai latihan C++ tingkat lanjut di Linux: manajemen sumber daya dengan RAII, `termios`, concurrency dengan `std::mutex` dan `std::condition_variable`, serta pengujian dengan GoogleTest dan ThreadSanitizer.

## Alur kerja

```
Port serial ──► SerialPort ──► FrameParser ──► ThreadPool ──► Logger ──► frames.log
 (termios)      (read bytes)   (state machine)  (N worker)     (mutex)
```

1. `SerialPort` membaca byte mentah dari perangkat atau port virtual.
2. `FrameParser` menyusun byte menjadi frame, dan membuang frame yang checksum-nya salah.
3. Setiap frame yang valid dikirim ke `ThreadPool`.
4. Worker mencatat frame ke file log melalui `Logger`, yang dilindungi mutex.

## Format frame

```
[0xAA] [0x55] [LEN] [PAYLOAD × LEN byte] [CHK]
```

- `0xAA 0x55`: penanda awal frame.
- `LEN`: panjang payload, 0 sampai 255.
- `CHK`: XOR dari `LEN` dan seluruh byte payload.

Byte di luar pola di atas diabaikan, sehingga parser bisa pulih dari data yang rusak.

## Struktur proyek

```
serial_pipeline/
├── CMakeLists.txt
├── include/serial/
│   ├── frame_parser.hpp
│   ├── logger.hpp
│   ├── serial_port.hpp
│   ├── thread_pool.hpp
│   └── thread_safe_queue.hpp
├── src/
│   ├── frame_parser.cpp
│   ├── serial_port.cpp
│   └── thread_pool.cpp
├── app/
│   └── main.cpp
└── tests/
    ├── frame_parser_test.cpp
    ├── logger_test.cpp
    ├── thread_pool_test.cpp
    └── thread_safe_queue_test.cpp
```

## Persyaratan

- Linux (diuji di Ubuntu)
- CMake 3.14 atau lebih baru
- GCC atau Clang dengan dukungan C++17
- Koneksi internet saat build pertama, karena GoogleTest diunduh otomatis lewat `FetchContent`

## Build

```bash
cmake -S . -B build
cmake --build build
```

## Menjalankan tes

```bash
./build/serial_tests
```

Semua tes harus lulus, ditandai dengan `[  PASSED  ]` di baris terakhir.

### Pemeriksaan data race

ThreadSanitizer tidak bisa digabung dengan AddressSanitizer, jadi gunakan folder build terpisah:

```bash
cmake -S . -B build-tsan -DCMAKE_CXX_FLAGS="-fsanitize=thread -g"
cmake --build build-tsan
./build-tsan/serial_tests
```

Tidak boleh ada `WARNING: ThreadSanitizer` di output.

## Menjalankan pipeline

```bash
./build/serial_demo [device] [file_log]
```

- `device`: path perangkat serial, default `/dev/ttyUSB0`.
- `file_log`: path file log, default `frames.log`.

Tekan `Ctrl+C` untuk menghentikan pipeline. Worker akan menyelesaikan frame yang sudah masuk sebelum program berhenti.

Untuk perangkat asli di Ubuntu, pengguna harus masuk grup `dialout`:

```bash
sudo usermod -aG dialout $USER
```

Lalu logout dan login ulang.

### Uji tanpa perangkat keras

Gunakan `socat` untuk membuat pasangan port virtual:

```bash
sudo apt install socat
socat -d -d pty,raw,echo=0,link=/tmp/vport1,mode=666 pty,raw,echo=0,link=/tmp/vport2,mode=666
```

Di terminal lain, jalankan pipeline pada `/tmp/vport1`:

```bash
./build/serial_demo /tmp/vport1 frames.log
```

Kirim frame valid dari terminal ketiga ke `/tmp/vport2`:

```bash
printf '\xaa\x55\x03\x01\x02\x03\x03' > /tmp/vport2
```

Baris baru di `frames.log` akan terlihat seperti ini:

```
1795326000000 | len=3 | 01 02 03
```

Frame dengan checksum salah tidak akan dicatat:

```bash
printf '\xaa\x55\x02\x0a\x0b\x00' > /tmp/vport2
```

## Komponen

| Komponen | Fungsi |
|---|---|
| `serial::SerialPort` | Membuka port dengan `termios` (mode raw, CS8). Menutup port secara otomatis di destruktor. |
| `serial::FrameParser` | Mesin state untuk menyusun frame dari potongan byte yang datang tidak beraturan. |
| `serial::ThreadSafeQueue<T>` | Antrean producer-consumer dengan `close()` untuk shutdown yang rapi. |
| `serial::ThreadPool` | Kumpulan worker thread. `shutdown()` menunggu tugas yang sudah masuk, dan aman dipanggil berkali-kali. |
| `serial::Logger` | Menulis baris log dengan timestamp dan payload dalam hex, dilindungi mutex. |

## Catatan desain

- **RAII di semua sumber daya.** File descriptor, thread, dan file log dibebaskan otomatis lewat destruktor.
- **Kepemilikan tunggal.** `SerialPort` dan `ThreadPool` tidak bisa disalin, hanya bisa dipindah.
- **Exception dari tugas tidak mematikan worker.** Worker menangkapnya dan melanjutkan ke tugas berikutnya.
- **Ctrl+C tidak memaksa keluar.** Handler `SIGINT` hanya mengubah flag, sehingga shutdown berjalan rapi.

## Lisensi

Proyek latihan pribadi. Silakan dipakai untuk belajar.