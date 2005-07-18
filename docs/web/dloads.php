<?php require "inc/common.php"; ?>
<?php head("spkg - downloads page"); ?>

 <h1>Downloads</h1>

  <p>On this page you can get the lastest version of the @SPKG@.</p>

 <h1>Binary package</h1>

  <p>I recommend you, to build @SPKG@ yourself. It's very easy, see 
  building section on this page.</p>

  <a href="/dl/spkg-@VER@-i486-1.tgz">binary package</a> for the 
  slackware-current is also available.</p>

 <h1>Source code</h1>

  <p>Source code is distributed as an ordinary <a
  href="/dl/spkg-@VER@.tar.gz">tarball</a>.</p>
  
  <p>Prerequisites for building @SPKG@ are:</p>
  <ul>
   <li><a href="http://www.sqlite.org">sqlite-3.2.2</a> - lightweight SQL database engine</li>
   <li><a href="http://www.gtk.org">glib-2.6.5</a> - general purpose C library</li>
  </ul>

 <h1>Building</h1>

  <p>Installing @SPKG@ is very easy. Just extract source code package
  and run  this command:</p>
  <code># make install</code>

  <p>To uninstall @SPKG@, do:</p>
  <code># make uninstall</code>
  
  <p>If you want to create Slackware binary package, do:</p>
  <code># make slackpkg</code>

<?php foot(); ?>
