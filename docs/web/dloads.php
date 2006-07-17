<?php require "common.php"; ?>
<?php head("spkg - downloads page"); ?>

 <h1>Downloads</h1>

  <p>On this page you can get the lastest version of the spkg.</p>

 <h2>Binary packages</h2>

  <p>I recommend you, to build spkg yourself. It's very easy, see 
  building section on this page. You can download latest binary package
  of <a href="dl/spkg-@VER@-i486-1.tgz">spkg</a> for slackware-current
  <a href="dl/">here</a>.</p>

 <h2>Source code</h2>

  <p>Source code is distributed as an ordinary <a
  href="dl/spkg-@VER@.tar.gz">tarball</a>. Prerequisites for building
  spkg are:</p>

  <ul>
   <li><a href="http://judy.sourceforge.net">Judy-1.0.3</a> - Judy arrays (heavily optimized arrays)</li>
   <li><a href="http://www.gtk.org">glib-2.10.3</a> - general purpose C library</li>
   <li><a href="ftp://ftp.rpm.org/pub/rpm/dist">popt-1.10.2</a> - command line parser</li>
   <li><a href="http://www.zlib.net">zlib-1.2.3</a> - zlib (de)compression library</li>
  </ul>

 <h1>Building</h1>

  <p>Installing spkg is very easy. Just extract source code package
  and run  this command:</p>
  <code># make install</code>

  <p>To uninstall spkg, do:</p>
  <code># make uninstall</code>
  
  <p>If you want to create Slackware binary package, do:</p>
  <code># make slackpkg</code>

<?php foot(); ?>
