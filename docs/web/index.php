<?php require "common.php"; ?>
<?php head("spkg - main page"); ?>

  <p style="font-size:130%;font-weight:bold;color:red;padding:4px;
  border:1px solid yellow;background:white;text-align:center;">
  @SPKG@-@VER@ "The Speedbringer" was released!</p>

 <h1>Intro</h1>

  <p>Welcome to the official website of @SPKG@, the unofficial
  <a href="http://slackware.com">Slackware Linux</a> package manager
  implementation.</p>

  <p>@SPKG@ is implemented in C and optimized for <strong>speed</strong>.</p>

  <p>See <a href="status.php">status</a> page for the current status
  and the roadmap of the @SPKG@ development.</p>

  <p style="border:1px solid yellow; background:white; padding: 4px;">
  <b>NOTE (2006-07-10):</b> spkg project developement was stalled for a year, but now
  I'm back continuing on it. What happened was, that I have got my first job
  exactly a year ago, partially thanks to my work on @SPKG@, I guess. :-)
  Some developement continued last summer in silence without release.
  Now I'm releasing alpha1 version of @SPKG@. This version features fully
  functional install and remove commands. More is comming soon! (hmm,
  hopefully sooner than next summer :-))</p>

 <h1>Features</h1>

  <ul>
    <li>Extreme symplicity. Just like pkgtools.</li>
    <li>Fast installation (approx. 10% faster than <strong>tar xzf</strong>)</li>
    <li>Fast unistallation (faster than <strong>rm -rf</strong>)</li>
    <li>Robust implementation. (nearly all error conditions are checked)</li>
    <li>Rollback functionality. (no file left behind policy ;-))</li>
    <li>Full compatibility with legacy Slackware package database.</li>
    <li>Everything is libified. (see <a href="docs.php">docs</a>) You can 
    implement new commands easily.</li>
    <li>Easy access to the package database thanks to libification.</li>
  </ul>

  <p><b>Planned features are:</b></p>

  <ul>
    <li>Fast upgrade, I guess... :-)</li>
    <li>Python bindings.</li>
    <li>PyGtk based GUI.</li>
  </ul>

 <h1>News</h1>

  <dl>
   <dt>2006-07-14</dt>
    <dd>I started publishing patches to the alpha1 version of @SPKG@
    in the <a href="dl/patches">patches</a> directory.</dd>
   <dt>2006-07-10</dt>
    <dd>Firtst alpha version released! This version implements install,
    remove and list commands. See manpage for more information.
    
    Other new features include fully functional command line interface
    with corresponding manpage and implementation of safe break points.
    
    You can safely break any command using some reasonable signal like
    SIGINT and all changes made so far will be automatically rolled back.
    
    There are new command verbosity selection options too.

    I've put spkg on diet and completely dropped sqlite and filedb
    database code. This results in fewer dependencies and little to
    no performance impact. Filedb code was replaced with JudySL
    arrays. See <a href="http://judy.sourceforge.net">this</a> page
    for more info.
    
    Enjoy it!</dd>

   <dt><a href="history.php">older news</a>...</dt>
  </dl>

<?php foot(); ?>
