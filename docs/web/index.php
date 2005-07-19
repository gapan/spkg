<?php require "inc/common.php"; ?>
<?php head("spkg - main page"); ?>

 <h1>Intro</h1>

  <p>Welcome to the official website of @SPKG@, the unofficial
  <a href="http://slackware.com">Slackware Linux</a> package manager
  implementation.</p>

  <p>@SPKG@ is implemented in C and optimized for <strong>speed</strong>.</p>


 <h1>Features</h1>

  <ul>
    <li>Fast installation (approx. 10% faster than <strong>tar xzf</strong>)</li>
    <li>Fast unistallation (faster than <strong>rm -rf</strong>)</li>
    <li>And yes, fast upgrade too. :-)</li>
    <li>Robust implementation. (nearly all error conditions are checked)</li>
    <li>Multiple security modes of command operation. (from paranoid up to brutal)</li>
    <li>Rollback functionality. (no file left behind policy ;-))</li>
    <li>Compatibility with legacy Slackware package database.</li>
    <li>Everything is librified. (see <a href="/docs.php">docs</a>) You can 
    implement new commands easily.</li>
    <li>Easy access to the package database thanks to Python bindings and 
    librification.</li>
  </ul>

 <h1>News</h1>

  <p style="font-size:140%;font-weight:bold;color:red;padding:0;
  border:1px solid yellow;background:white;text-align:center;">@SPKG@ 
  @VER@ released!</p>

  <p>See <a href="/status.php">status</a> page for the current status
  and the roadmap of the @SPKG@ development.</p>

  <dl>
   <dt>2005-07-20</dt>
    <dd>Firtst alpha version released! This version implements install
    command with four modes of operation: brutal, normal, cautious and
    paranoid. See documentation for more information. Other new features
    include fully functional command line interface with correpsonding
    manpage and implementation of safe break points. You can safely break
    installation using some reaonable signal like SIGINT and all changes
    made so far during installation will be automatically rolled back.</dd>

   <dt><a href="/history.php">older news</a>...</dt>
  </dl>

<?php foot(); ?>
