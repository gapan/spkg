<?php require "inc/common.php"; ?>
<?php head("spkg - howto page"); ?>

 <h1>@SPKG@ howto</h1>

  <p>This page explains how to get started with @SPKG@.</p>

 <h1>Quick start</h1>

  <p>After successful installation you need to create @SPKG@ database
  from your current slackware package database. (legacy database)</p>

  <code># spkg --sync-cache</code>

  <p>That's all. Now you can use @SPKG@ to install packages.</p>

  <code># spkg --verbose --install --mode=paranoid some-package-1.0-i486-1.tgz</code>
  
  <p>If you are lazy typer, previous command can be rewritten to:</p>

  <code># spkg -vimp some-package-1.0-i486-1.tgz</code>

  <p>For more info see help:</p>

  <code># spkg --help</code>

  <p>or manpage:</p>

  <code># man 8 spkg</code>

<?php foot(); ?>
