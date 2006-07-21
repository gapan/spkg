<?php require "common.php"; ?>

 <h2>Downloads</h2>

  <p>On this page you can get the lastest version of the spkg:
  <b>spkg-<?php echo $version; ?></b></p>

 <h3>Binary packages</h3>

  <p>I recommend you, to build spkg yourself. It's very easy, see 
  building section on this page. You can also download the latest binary
  package: <a href="dl/releases/spkg-<?php echo $version; ?>-i486-1.tgz">
  spkg-<?php echo $version; ?>-i486-1.tgz</a>.</p>

 <h3>Source code</h3>

  <p>Source code is distributed as an ordinary tarball. Tarball for
  the latest version is: <a href="dl/releases/spkg-<?php echo $version; ?>.tar.gz">
  spkg-<?php echo $version; ?>.tar.gz</a>.</p>

 <h3>More files</h3>

  <p>You can find more files in <a href="dl">this</a> directory.</p>

 <h2>Building</h2>

  <p>Prerequisites for building spkg are:</p>

  <ul>
   <li><a href="http://judy.sourceforge.net">Judy-1.0.3</a> - Judy arrays (heavily optimized arrays)</li>
   <li><a href="http://www.gtk.org">glib-2.10.3</a> - general purpose C library</li>
   <li><a href="ftp://ftp.rpm.org/pub/rpm/dist">popt-1.10.2</a> - command line parser</li>
   <li><a href="http://www.zlib.net">zlib-1.2.3</a> - zlib (de)compression library</li>
  </ul>

  <p>Installing spkg is very easy. Just extract source code package
  and run this command (you will need at least to have Judy first):</p>
  <code># make install</code>

  <p>To uninstall spkg, do:</p>
  <code># make uninstall</code>
  
  <p>If you want to create Slackware binary package, do:</p>
  <code># make slackpkg</code>

<?php foot(); ?>
