<?php require "common.php"; ?>

 <h2>Spkg HOWTO</h2>

  <p>This page explains how to get started with spkg.</p>

 <h3>Quick start</h3>

  <p>After successful <a href="dload.php">installation</a> you can start
  using spkg to install or upgrade existing packages:</p>
  <code># spkg some-package-1.0-i486-1.tgz</code>
  
  <p>To remove existing package do:</p>
  <code># spkg -d some-package</code>

  <p>For more info see help:</p>
  <code># spkg --help</code>

  <p>or manpage:</p>
  <code># man spkg</code>

<?php foot(); ?>
