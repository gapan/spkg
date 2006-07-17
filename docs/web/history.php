<?php require "common.php"; ?>
<?php head("spkg - history"); ?>

 <h1>History</h1>

   <h2>2006-07-14</h2>
    <p>I started publishing patches to the alpha1 version of spkg
    in the <a href="dl/patches">patches</a> directory.</p>
   <h2>2006-07-10</h2>
    <p>Firtst alpha version released! This version implements install,
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
    
    Enjoy it!</p>
   <h2>2005-07-17</h2>
    <p>Install command is nearly completed. spkg installs 60 packages
    (total size 18MB) under 3 seconds. Installpkg from pkgtools needs 90
    seconds to install the same set of packages. Whooooa! Maybe I should
    reabbreviate spkg for speeeeedy package manager. :] Keep tuned.</p>
   <h2>2005-06-29</h2>
    <p>Pkgdb completed.</p>
   <h2>2005-06-27</h2>
    <p>Filedb library received new features: fast distributed
    checksumming and per file arbitrary data storing. It is possible
    to open multiple file databases at once. Implemented new, much
    better error handling everywhere. <a href="dl/TODO">TODO</a>
    file is now automatically updated on the web.</p>
   <h2>2005-06-21</h2>
    <p>I've implemented <a href="dl/spkg-docs/pyspkg.html">Python
    bindings</a>. Now I'm finishing pkgdb library.</p>
   <h2>2005-06-14</h2>
    <p>SQL database wrapper API completed. Untgz documented.</p>
   <h2>2005-06-12</h2>
    <p>Another milestone achieved. Pkgdb is fast. Database operations on 
    averange package can be done in 2ms on my 1GHz Athlon. Full 
    synchronization with legacy database on my system can be done in 1 second.
    (522 packages)</p>
   <h2>2005-06-10</h2>
    <p>Website created. Development code released.</p>
  </dl>

<?php foot(); ?>
