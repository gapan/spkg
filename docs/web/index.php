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
    simultaneously.</li>
    <li>Everything is librified. (see <a href="/docs.php">docs</a>) You can implement
    new commands easily.</li>
    <li>Easy access to the package database thanks to Python bindings.</li>
  </ul>

 <h1>News</h1>

  <p>The lastest version of @SPKG@ is <strong>@VER@</strong>.</p>
 
  <p>See <a href="/status.php">status</a> page for the current status
  and roadmap of the @SPKG@ development.</p>

  <dl>
   <dt>2005-06-25</dt>
    <dd>Filedb library implementats autogrow, locking, fast distributed
    checksumming and per file storing of arbitrary data. It is possible
    to open multiple filedb databases at once. Pkgdb library is finished.</dd>
   <dt>2005-06-21</dt>
    <dd>I've implemented <a href="/dl/spkg-docs/pyspkg.html">Python
    bindings</a>. Now I'm finishing pkgdb library.</dd>
   <dt>2005-06-14</dt>
    <dd>SQL database wrapper API completed. Untgz documented.</dd>
   <dt>2005-06-12</dt>
    <dd>Another milestone achieved. Pkgdb is fast. Database operations on 
    averange package can be done in 2ms on my 1GHz Athlon. Full 
    synchronization with legacy database on my system can be done in 1 second.
    (522 packages)</dd>
   <dt>2005-06-10</dt>
    <dd>Website created. Development code released.</dd>
  </dl>

<?php foot(); ?>
