# NÖBET // Uzun Koridor

Bu depo, **C++17 ile yazılmış, harici kütüphane gerektirmeyen özgün bir terminal korku macerasıdır**. Roblox DOORS'un kodunu, markasını, karakterlerini veya varlıklarını kopyalamaz; oda-oda ilerleme, eşya arama, saklanma ve kovalamaca gibi benzer tür mekaniklerini kendi isimleri ve kurallarıyla uygular.

## Atmosfer

Oyun artık yalnızca metin basmaz:

- ANSI renkleriyle durum paneli, rota şeridi ve oda tipine göre değişen ASCII sahneleri çizer.
- Karanlık oda, tehlike ve kovalamaca ekranları farklı renk/çizim kullanır.
- `assets/sfx/` altında kapı, anahtar, eşya, çakmak, tehlike, saklanma, darbe, kovalamaca, zafer ve ölüm için özgün WAV efektleri bulunur.
- Linux/macOS'ta `paplay`, `aplay`, `afplay` veya `ffplay` varsa efektleri otomatik çalar. Hiçbiri yoksa etkileşimli terminalde sistem ziliyle geri bildirim verir.
- Sessiz oynamak için `--no-audio` kullanabilir veya `NO_SOUND=1` ayarlayabilirsin.

Sesleri yeniden üretmek/değiştirmek için:

```bash
python3 tools/generate_audio.py
```

Bu script yalnızca Python standart kütüphanesini kullanır.

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

Windows'ta `nightshift.exe` çalıştırılabilir. Windows ve ses oynatıcı bulunmayan sistemlerde ASCII/renkler yine çalışır.

## Kullanım

```text
./nightshift --doors 30 --seed 12345
```

- `--doors SAYI`: prosedürel binanın uzunluğu (12–100)
- `--seed SAYI`: aynı haritayı ve ganimet düzenini yeniden üretmek için tohum
- `--no-audio`: ses efektlerini ve terminal zilini kapat

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
