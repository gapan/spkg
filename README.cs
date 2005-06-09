spkg - rychlı balíèkovací mana¾er pro Slackware Linux
-----------------------------------------------------

Popis
-----
Balíèkovací mana¾er je program pro správu balíèkù. Správou
se rozumí instalace, aktualizace, odstraòování a kontrola
balíèkù.

Balíèek
-------
Obecnì je balíèek mno¾ina souborù a informací spoleènıch tìmto 
souborùm (metadat). Pøíkladem metadat mù¾e bıt skript, 
definující pøíkazy, které se mají provést po urèité akci, 
seznam závislostí na jinıch balíècích, atp.

Formát balíèku
--------------
Ka¾dı balíèek lze identifikovat pomocí jednoznaèného
identifikátoru, kterı se skládá z:
  - názvu balíèku
  - verze balíèku
  - architektury pro kterou je balíèek urèen
  - èísla sestavení balíèku
  - id autora

Pozn.: Nìkdy se v kódu identifikátor oznaèuje jako [dlouhı] název 
balíèku a název balíèku jako krátkı název balíèku. (name a shortname)

Identifikátor má formát (¹pièaté závorky obsahují povinné èásti, hranaté
závorky nepovinné èásti):
  <název>-<verze>-<architektura>-<sestavení>[autor]

®ádná èást identifikátoru, kromì názvu nemù¾e obsahovat pomlèku.

Balíèek je gzipem komprimovanı tar archiv obsahující dva typy
souborù:
  - soubory s metadaty:        install/slack-* install/doinst.sh
  - soubory balíèkovanıch dat: zbylé soubory

®ádnı soubor v archivu nesmí mít absolutní cestu. (tj. cestu
zaèínající lomítkem)

Soubory metadat by mìly bıt umístìny co nejblí¾e k zaèátku archivu.

doinst.sh: Skript shellu, kterı se spou¹tí po úspì¹né extrakci
souborù. Skript se spu¹tí v koøenovém adresáøi balíèku. Provádìné 
pøíkazy nesmìjí modifikovat soubory v rodièovskıch adresáøích
aktuálního adresáøe. Z toho vyplívá, ¾e autor skriptu nesmí 
pøedpokládat, ¾e bude skript spu¹tìn v koøenovém adresáøi (/).

slack-desc: Je soubor obsahující krátkı a dlouhı popis balíèku. 
První platnı øádek souboru musí mít formát:
  <název> (krátkı popis)

Dále soubor obsahuje maximálnì 10 takovıchto øádkù:
  [dlouhı popis]

Platnı øádek je øádek zaèínající textem: "<název>:"

Pøíklad platného souboru slack-desc:

spkg: spkg (fast package manager)
spkg: spkg is The Fastest Package Managment Tool on the world.
spkg: Written by Ondrej Jirman <megous@megous.com>

spkg
----
Hlavním úkolem balíèkového mana¾eru je udr¾ovat databázi nainstalovanıch
balíèkù a k nim pøíslu¹ejících souborù. 

