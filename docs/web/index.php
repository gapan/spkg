<?php require "inc/common.php"; ?>
<?php head("spkg - The Unofficial Slackware Linux Package Manager"); ?>

 <h1>Intro</h1>

  <p>Welcome to the official website of @SPKG@, the unofficial
  <a href="http://slackware.com">Slackware Linux</a> package manager
  implementation.</p>

  <p>@SPKG@ is implemented in C and optimized for <strong>speed</strong>.</p>

 <h1>Features</h1>

  <ul>
    <li>Fast installation (just as fast as <strong>tar xzf</strong>)</li>
    <li>Fast unistallation (just as fast as <strong>rm -rf</strong>)</li>
    <li>And yes, fast upgrade too. :-)</li>
    <li>Compatibility with legacy Slackware package database. This means, that you
    may simply use both @SPKG@ and original <strong>pkgtools</strong> 
    simultaneosuly.</li>
    <li>Everything is librified. (see <a href="/docs.php">docs</a>) You can implement
    new commands easily.</li>
  </ul>

 <h1>News</h1>

  <p>The lastest version of @SPKG@ is <strong>@VER@</strong>.</p>
 
  <p>See <a href="/status.php">status</a> page for the current status
  and roadmap of the @SPKG@ development.</p>

  <dl>
   <dt>2005-06-10</dt>
    <dd>Website created. Development code released.</dd>
  </dl>

<?php foot(); ?>
