<?php require "common.php"; ?>

  <p>Welcome to the official website of spkg, the unofficial <a
  href="http://slackware.com">Slackware Linux</a> package manager. spkg
  is implemented in C and optimized for speed. The latest version
  relased on <?php echo $reldate; ?> is: <b><a href="dl/releases">
  spkg-<?php echo $version; ?></a></b>. See <a href="dl/NEWS">NEWS</a>
  file for more information about this release.</p>

  <p><b>Spkg is now feature complete and under heavy testing by yours
  truly.</b></p>

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

 <h2>News</h2>

  <h3>Spkg is heading towards 1.0</h3>

   <p>What this means is, that I've modified site to allow me to easily
   roll out spkg-1.0 release candidates. So this means, that there
   will be a lot of them. :-)</p>

   <p>I've tested beta a lot and come up with some benchmarks you
   can find <a href="dl/BENCHMARKS">here</a>. Just a few numbers
   comparing pkgtools to spkg here: installation is at least 4x faster,
   upgrade is 7x faster and remove is 30x faster on averange.</p>
   

  <p>See <a href="history.php">older news</a>...</p>

<?php foot(); ?>
