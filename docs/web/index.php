<?php require "common.php"; ?>

  <p>Welcome to the official website of spkg, the unofficial <a
  href="http://slackware.com">Slackware Linux</a> package manager. spkg
  is implemented in C and optimized for speed. The latest version
  relased on <?php echo $reldate; ?> is: <b><a href="dl/releases">
  spkg-<?php echo $version; ?></a></b>. See <a href="dl/NEWS">NEWS</a>
  file for more information about this release.</p>

  <p>Spkg is used in <a href="http://www.salixos.org">Salix OS</a> to 
  make its package installation and upgrades blazingly fast.</p>

 <h2>News</h2>

  <h3>Spkg 1.0 was released</h3>

  <p>After almost five years since the development started, spkg reached version 1.0. Yay!</p>

  <p>Over those five years, spkg received some commercial support by <a href="http://zonio.net">Zonio</a>, 
    gained windows support thanks to <a href="http://mingw.org/wiki/LibrariesAndTools">Laura Michaels</a>, 
  and was adopted by Salix OS developers for package management in their 
  distribution.</p>

  <h3>Spkg GIT repository on GITHUB</h3>

   <p>Spkg GIT repository is available on <a
   href="http://github.com/megous/spkg">GITHUB</a>. Feel free to
   create forks and send me pull requests. ;-)</p>

  <p>See <a href="history.php">older news</a>...</p>

 <h2>Features</h2>

  <ul>
    <li>Simple user interface. Just like pkgtools.</li>
    <li>Fast install, upgrade and remove operations.</li>
    <li>Command to list information about installed packages.</li>
    <li>Dry-run mode, in which filesystem is not touched.</li>
    <li>Safe mode for installing untrusted packages.</li>
    <li>Rollback and safe cancel functionality.</li>
    <li>You can use spkg and pkgtools side by side.</li>
    <li>You can be informed about all actions spkg does if you turn on verbose mode.</li>
    <li>Everything is libified. (see <a href="docs.php">docs</a>) You can 
    implement new commands easily.</li>
  </ul>

   <p>You can find some benchmarks <a href="dl/BENCHMARKS">here</a>. Here are 
    just a few numbers
   comparing pkgtools to spkg: installation is at least 4x faster,
   upgrade is 7x faster and remove is 30x faster on averange.</p>
   

<?php foot(); ?>
