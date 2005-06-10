<?php require "inc/common.php"; ?>
<?php head("spkg - The Unofficial Slackware Linux Package Manager"); ?>

 <h1>Intro</h1>

  <p>Welcome to the official website of <strong>spkg</strong>, the unofficial
  <a href="http://slackware.com">Slackware Linux</a> package manager.</p>

  <p>Spkg is implemented in C, optimized for speed and reliability.</p>

 <h1>Features</h1>

  <ul>
    <li>Fast installation (just as fast as <strong>tar xzf &lt;pkg&gt;</strong>)</li>
    <li>Fast unistallation (just as fast as <strong>rm -rf</strong>)</li>
    <li>And yes, fast upgrade too. :-)</li>
    <li>Compatibility with legacy Slackware package database. This measns, that you
    may simply use both <strong>spkg</strong> and original <strong>pkgtools</strong> 
    simultaneosuly.</li>
    <li>Everything is librified. (see <a href="/docs.php">docs</a>) You can write new commands easily.</li>
  </ul>

 <h1>News</h1>

  <dl>
   <dt>2005-06-09</dt>
    <dd>Website created. Development code released.</dd>
  </dl>

<?php foot(); ?>
