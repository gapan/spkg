<?php require "common.php"; ?>
<?php head("spkg - howto page"); ?>

 <h1>@SPKG@ HOWTO</h1>

  <p>This page explains how to get started with @SPKG@.</p>

 <h1>Quick start</h1>

  <p>After successful installation you can start installing and removing
  packages using following commands:</p>
  <code># spkg --install some-package-1.0-i486-1.tgz</code>
  
  <p>If you are a lazy typer, previous command can be rewritten to:</p>
  <code># spkg -i some-package-1.0-i486-1.tgz</code>

  <p>And for total lazy-bones, we have even simpler command:</p>
  <code># ipkg some-package-1.0-i486-1.tgz</code>

  <p>You can remove installed package using this command:</p>
  <code># rpkg some-package</code>

  <p>For more info see help:</p>
  <code># spkg --help</code>

  <p>or manpage:</p>
  <code># man spkg</code>

<?php foot(); ?>
