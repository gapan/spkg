<?php require "common.php"; ?>
<?php head("spkg - status page"); ?>

 <h1>Status</h1>

  <p>The lastest version of @SPKG@ is <strong>@VER@</strong>.</p>

  <p>@SPKG@ is now under active development. See <a href="dl/ChangeLog">Change Log</a>
  and <a href="dl/TODO">TODO</a> for details.</p>

 <h1>Roadmap</h1>

  <p>Here is the current roadmap/history of the @SPKG@ development.</p>

  <table cellspacing="0">
    <tr><th>date</th><th>milestone</th></tr>
<?php
  $milestones = array(
    array(1,"first idea of pkgtools implementation in C", "2005-04-01"),
    array(1,"first code is commited to an archive", "2005-04-20"),
    array(1,"extremly fast filedb library is implemented", "2005-06-01"),
    array(1,"existing codebase is stabilized", "2005-06-04"),
    array(1,"website created", "2005-06-09"),
    array(1,"pkgdb library is fully implemented", "2005-06-29"),
    array(1,"command line interface is fully implemented", "2005-07-20"),
    array(1,"alpha1 release (install, remove and list commands)", "2006-07-10"),
    array(0,"alpha2 release (all commands implemented)", "? 2006-08-01 ?"),
    array(0,"beta release (feature complete including python bindings)", "? 2006-08-?? ?"),
    array(0,"first stable version is released (after heavy testing in production environment)", "???"),
  );
  foreach($milestones as $m)
  {
    echo "    <tr class=\"".($m[0]?"p":"f")."\">".
         "<td class=\"date\">${m[2]}</td><td>${m[1]}</td></tr>\n";
  }
?>
  </table>

<?php foot(); ?>
