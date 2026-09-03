# NÖBET // Uzun Koridor

Bu depo, **C++17 ile yazılmış, harici kütüphane gerektirmeyen özgün bir terminal korku macerasıdır**. Roblox DOORS'un kodunu, markasını, karakterlerini veya varlıklarını kopyalamaz; oda-oda ilerleme, eşya arama, saklanma ve kovalamaca gibi benzer tür mekaniklerini kendi isimleri ve kurallarıyla uygular.

## Atmosfer

Oyun artık yalnızca metin basmaz:

- ANSI renkleriyle durum paneli, rota şeridi ve oda tipine göre değişen ASCII sahneleri çizer.
- Karanlık oda, tehlike ve kovalamaca ekranları farklı renk/çizim kullanır.
- Etkileşimli terminalde açılış, kapı geçişi ve tehlike için kısa animasyonlar gösterilir.
- İki ayrı kovalamaca bölümü, zorluk seviyesine göre farklı uzunlukta oynanır.
- Her odada keşfedilebilir rota şeridi, oda plakası ve oda tipine özel küçük sahne bulunur.
- Bazı kapılar yalnızca anahtarla değil; sigorta paneli veya üç haneli sayı kilidi çözülerek açılır.
- Revirlerde `dinlen` komutuyla sınırlı sağlık/akıl yenilemesi yapılabilir.
- Bazı kapılar yankı vermeyen yanıltıcı kapılardır; `dinle` komutunu kullanmak hasar almamanı sağlar.
- Servis arabalarında jetonlarla destek eşyaları satın alınabilir.
- `assets/sfx/` altında kapı, anahtar, eşya, çakmak, tehlike, saklanma, darbe, panel, hata, ortam, kovalamaca, zafer ve ölüm için özgün WAV efektleri bulunur.
- Linux/macOS'ta `paplay`, `aplay`, `afplay` veya `ffplay` varsa efektleri otomatik çalar. Hiçbiri yoksa etkileşimli terminalde sistem ziliyle geri bildirim verir.
- Sessiz oynamak için `--no-audio` kullanabilir veya `NO_SOUND=1` ayarlayabilirsin.

Sesleri yeniden üretmek/değiştirmek için:

```bash
python3 tools/generate_audio.py
# veya
make sounds
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
./nightshift --doors 30 --seed 12345 --difficulty standart
```

- `--doors SAYI`: prosedürel binanın uzunluğu (12–100)
- `--seed SAYI`: aynı haritayı ve ganimet düzenini yeniden üretmek için tohum
- `--difficulty rahat|standart|kabus`: tehlike olasılığı, hasar ve kovalamaca uzunluğu
- `--no-audio`: ses efektlerini ve terminal zilini kapat

Oyun içindeki temel komutlar:

- `yardım` — komut listesini gösterir
- `bak` — odayı incele
- `ara` — ganimet, anahtar ve ipuçlarını bul
- `kapıyı aç` veya `ileri` — sonraki kapıya geç
- `geri` — bir önceki odaya dön
- `saklan` / `çık` — dolaba gir veya çık
- `kullan bandaj`, `kullan çakmak`, `kullan maymuncuk`, `kullan sigorta` — eşya kullan
- `çöz 314` — sayı kilidini aç; kodu odadaki ipucunda ara
- `satın al bandaj` — servis arabasından jetonla eşya al
- `çanta`, `durum`, `dinle`, `dinlen`
- `kaydet`, `yükle`, `çıkış`

Karanlık odalarda arama yapmadan önce çakmağı yakmak, kilitli kapılardan önceki odayı tekrar aramak ve ekrandaki kaçış hareketini doğru yazmak gerekir.
