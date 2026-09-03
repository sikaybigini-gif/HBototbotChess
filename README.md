# NÖBET // Uzun Koridor

Bu depo, **C++17 ile yazılmış, harici kütüphane gerektirmeyen özgün bir co-op korku macerasıdır**. Roblox DOORS'un kodunu, markasını, karakterlerini veya varlıklarını kopyalamaz; oda-oda ilerleme, eşya arama, saklanma ve kovalamaca gibi benzer tür mekaniklerini kendi isimleri ve kurallarıyla uygular.

## Tek oyunculu sürüm

```bash
make
./nightshift --doors 30 --seed 12345 --difficulty standart
```

- `--doors SAYI`: prosedürel binanın uzunluğu (12–100)
- `--seed SAYI`: aynı haritayı ve ganimet düzenini yeniden üretmek için tohum
- `--difficulty rahat|standart|kabus`: tehlike olasılığı, hasar ve kovalamaca uzunluğu
- `--no-audio`: ses efektlerini ve terminal zilini kapat

## Arkadaşlarla oynama: bilgisayarı sunucu yapma

Sunucu bilgisayarda:

```bash
make
./nightshift-server --port 8080 --doors 20
```

Ardından sunucu bilgisayarında tarayıcıdan `http://localhost:8080` adresini aç. Arkadaşların aynı Wi-Fi ağındaysa sunucu bilgisayarın yerel IP adresini kullanır:

```text
http://SUNUCU_BILGISAYARININ_IPSI:8080
```

Örnek: `http://192.168.1.42:8080`

1. İlk oyuncu yeni lobi oluşturur.
2. Ekrandaki dört karakterli lobi kodunu arkadaşlarına gönderir.
3. Diğer oyuncular kodu girerek aynı asansöre biner.
4. Herkes `HAZIRIM` dedikten sonra host `ASANSÖRÜ BAŞLAT` düğmesine basar.
5. En fazla 8 kişi aynı odayı, bulmacaları, tehditleri ve kovalamacaları paylaşır.
6. Ekip kapılarında iki farklı oyuncunun aynı kapıya basması gerekir.
7. Sohbet paneliyle ekip içi mesajlaşabilir; bandaj/adrenalin kullanarak kaybolan arkadaşını canlandırabilirsin.
8. Hazır olmadan önce **rol seçiciden** ekibine uygun bir uzmanlık seçebilirsin.

### Ekip rolleri

Her oyuncu lobi ekranından bir rol seçer. Roller yalnızca oyunun kurallarını değiştirir; isimler, görseller ve mekanikler NÖBET'e özgüdür.

| Rol | Ekip avantajı |
| --- | --- |
| **Keşifçi** | Yankısız yanıltıcı kapıyı ilk denemede hasar almadan fark eder. |
| **Sağlıkçı** | Canlandırdığı oyuncuyu daha yüksek can ve akılla döndürür; revire dinlenmeye girdiğinde yakındaki ekibe küçük bir sağlık desteği verir. |
| **Mühendis** | Kilit açarken kullandığı maymuncuğu tüketmez. |
| **Muhafız** | Gürültü ve fısıltı tehditlerinden yaklaşık üçte bir daha az hasar alır. |

Roller kapı açıldıktan sonra da oyuncu kartlarında görünür; böylece ekip kimin hangi avantajı taşıdığını takip edebilir.

Sunucu `0.0.0.0` adresine bağlanır; bu yüzden telefonlar aynı yerel ağ üzerinden erişebilir. İşletim sistemi güvenlik duvarında TCP `8080` portuna izin vermen gerekebilir. İnternet üzerinden oynatmak için port yönlendirme yerine Tailscale/ZeroTier gibi güvenilir bir VPN tercih et. Bu prototipte hesap sistemi ve kimlik doğrulama yoktur; herkese açık internete doğrudan açma.

## Mobil destek

`web/` içindeki arayüz mobil önceliklidir:

- Dokunmaya uygun büyük düğmeler
- Canvas tabanlı birinci şahıs pseudo-3D oda görünümü; perspektif kapı, ışık, zemin ve gölge çizimleri
- Oda değişiminde kararma geçişi, merkez retikülü, CRT tarama çizgileri ve kovalamacada kamera sarsıntısı
- Responsive tek sütun görünümü
- Güvenli ekran boşlukları ve mobil viewport ayarları
- Telefon titreşimi ile tehlike uyarısı
- Tarayıcı içi WAV ses efektleri
- Ana ekrana eklenebilen basit PWA manifesti
- Durum, ekip listesi ve kapı rotası telefonda ayrı paneller halinde

Sunucu, web istemcisine HTTP üzerinden küçük ve yetkili oyun durumları gönderir; istemci yaklaşık 700 ms'de bir günceller. Böylece WebSocket kütüphanesi gerektirmeden LAN ve mobil tarayıcılarda çalışır.

## Atmosfer ve mekanikler

- Terminal sürümünde ANSI renkleri, durum paneli, rota şeridi ve oda tipine göre değişen ASCII sahneler
- Web sürümünde perspektifli Canvas sahnesi, oda tipine özgü dekor, ışık titremesi, tehdit gölgesi ve chase vinyeti
- Etkileşimli terminalde açılış, kapı geçişi ve tehlike animasyonları
- İki ayrı kovalamaca bölümü
- Sigorta paneli ve üç haneli sayı kilidi bulmacaları
- Yankı vermeyen yanıltıcı kapılar; `dinle` ile fark edilir
- Revirlerde `dinlen` ile sınırlı sağlık/akıl yenilemesi
- Servis arabalarında jetonlarla destek eşyası satın alma
- Karanlık odalarda çakmak kullanma
- `kaydet` / `yükle` desteği

## Sesleri üretme

`assets/sfx/` altında kapı, anahtar, eşya, çakmak, tehlike, saklanma, darbe, panel, hata, ortam, kovalamaca, zafer ve ölüm için özgün WAV efektleri bulunur. Linux/macOS'ta `paplay`, `aplay`, `afplay` veya `ffplay` varsa terminal sürümü bunları otomatik çalar. Hiçbiri yoksa etkileşimli terminalde sistem zili kullanılır.

Sesleri yeniden üretmek/değiştirmek için:

```bash
python3 tools/generate_audio.py
# veya
make sounds
```

Bu script yalnızca Python standart kütüphanesini kullanır. Web sürümü aynı WAV dosyalarını tarayıcıda çalar.

## Derleme seçenekleri

Linux, macOS veya MinGW üzerinde:

```bash
make                 # nightshift + nightshift-server
make run             # tek oyunculu terminal sürümü
make server          # 8080 portunda co-op web sunucusu
```

CMake ile:

```bash
cmake -S . -B build
cmake --build build
./build/nightshift-server --port 8080
```

Windows'ta `nightshift.exe` ve `nightshift-server.exe` çalıştırılabilir; sunucu Winsock kullanır.

## Oyun içi komutlar

Tek oyunculu terminal sürümünde:

- `yardım`, `bak`, `ara`
- `kapıyı aç`, `ileri`, `geri`
- `saklan`, `çık`, `dinle`, `dinlen`
- `kullan bandaj`, `kullan çakmak`, `kullan maymuncuk`, `kullan sigorta`
- `çöz 314` — sayı kilidini aç; kodu ipucunda ara
- `satın al bandaj` — servis arabasından jetonla eşya al
- `çanta`, `durum`, `kaydet`, `yükle`, `çıkış`

Web sürümünde aynı mekanikler büyük dokunmatik düğmelere dönüştürülmüştür.
