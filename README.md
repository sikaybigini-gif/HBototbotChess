# NÖBET // Uzun Koridor

Bu depo, **C++17 ile yazılmış, harici kütüphane gerektirmeyen özgün bir terminal korku macerasıdır**. Roblox DOORS'un kodunu, markasını, karakterlerini veya varlıklarını kopyalamaz; oda-oda ilerleme, eşya arama, saklanma ve kovalamaca gibi benzer tür mekaniklerini kendi isimleri ve kurallarıyla uygular.

## Derleme

Linux, macOS veya MinGW üzerinde:

```bash
make
./nightshift
```

CMake ile:

```bash
cmake -S . -B build
cmake --build build
./build/nightshift
```

Windows'ta `nightshift.exe` çalıştırılabilir.

## Kullanım

```text
./nightshift --doors 30 --seed 12345
```

- `--doors 12-100`: prosedürel binanın uzunluğu
- `--seed`: aynı haritayı ve ganimet düzenini yeniden üretmek için tohum

Oyun içindeki temel komutlar:

- `yardım` — komut listesini gösterir
- `bak` — odayı incele
- `ara` — ganimet, anahtar ve ipuçlarını bul
- `kapıyı aç` veya `ileri` — sonraki kapıya geç
- `geri` — bir önceki odaya dön
- `saklan` / `çık` — dolaba gir veya çık
- `kullan bandaj`, `kullan çakmak`, `kullan maymuncuk` — eşya kullan
- `çanta`, `durum`, `dinle`
- `kaydet`, `yükle`, `çıkış`

Karanlık odalarda arama yapmadan önce çakmağı yakmak, kilitli kapılardan önceki odayı tekrar aramak ve ekrandaki kaçış hareketini doğru yazmak gerekir.
