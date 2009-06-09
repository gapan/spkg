<?php require "common.php"; ?>

 <h2>Downloads</h2>

  <p>On this page you can get the lastest version of the spkg:
  <b>spkg-<?php echo $version; ?></b></p>

 <h3>Binary packages</h3>

  <p>I recommend you to build spkg yourself. It's very easy as you can see 
  below. Binary packages for linux are no longer available. Package for windows
  can be downloaded here: <a href="dl/releases/spkg-<?php echo $version; ?>-win32-bin.tar.gz">
  spkg-<?php echo $version; ?>-win32-bin.tar.gz</a>.</p>

 <h3>Source code</h3>

  <p>Source code is distributed as an ordinary tarball. Tarball for
  the latest version is: <a href="dl/releases/spkg-<?php echo $version; ?>.tar.gz">
  spkg-<?php echo $version; ?>.tar.gz</a>.</p>

 <h3>More files</h3>

  <p>You can find more files in <a href="dl">this</a> directory.</p>

 <h2>Building</h2>

  <p>Prerequisites for building spkg are at least:</p>

  <ul>
   <li><a href="http://judy.sourceforge.net">Judy-1.0.3</a> - Judy arrays (heavily optimized arrays)</li>
   <li><a href="http://www.gtk.org">glib-2.2.1</a> - general purpose C library</li>
   <li><a href="ftp://ftp.rpm.org/pub/rpm/dist">popt-1.7</a> - command line parser</li>
   <li><a href="http://www.zlib.net">zlib-1.1.4</a> - zlib (de)compression library</li>
  </ul>

  <p>NOTE: It may compile/work even with older version of these libraries but it was not tested.</p>

  <p>Installing spkg is very easy. Just extract source code package
  and run these commands (you will need to have Judy installed first):</p>
  <code># ./configure && make && make install</code>

  <p>To uninstall spkg, do:</p>
  <code># make uninstall</code>
  
<?php foot(); ?>
