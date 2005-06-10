<?php require "inc/common.php"; ?>
<?php head("spkg - The Unofficial Slackware Linux Package Manager"); ?>

 <h1>Downloads</h1>
  <p>On this page you can get the lastest version of the <strong>spkg</strong>.</p>

 <h1>Source code</h1>
  <p>Source code is distributed as ordinary tar archive.</p>
  
  <p>Prerequisites for building spkg are:</p>
  <ul>
   <li><a href="http://www.sqlite.org">sqlite3</a> - lightweight SQL database engine</li>
   <li></li>
   <li></li>
  </ul>

 <h1>Building</h1>
  <p>Installing spkg is very easy. Just extract source code package and run 
  this command:</p>
  <code># make install</code>

  <p>To uninstall spkg, do:</p>
  <code># make uninstall</code>
  
  <p>If you want to create Slackware package for installation on more computers, do:</p>
  <code># make slackpkg</code>

<?php foot(); ?>
