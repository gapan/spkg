<?php require "inc/common.php"; ?>
<?php head("spkg - The Unofficial Slackware Linux Package Manager"); ?>

 <h1>Status (2005-06-10)</h1>

  <p>The lastest version of @SPKG@ is <strong>@VER@</strong>.</p>

  <p>spkg is under active development. Main parts of the @SPKG@ core
  library are completed.</p>
  
  <table cellspacing="0">
    <tr><th>library part</th><th>docs</th><th>code</th><th>status description</th></tr>
<?php
  $status = array(
    array("untgz",   100, 100,  "completed!"),
    array("sql",     100, 100,  "completed!"),
    array("filedb",   40,  80,  "atomic transactions missing"),
    array("pkgdb",    60,  90,  "a whole lot of XXXs to be removed"),
    array("taction",   0,  10,  "only code skeleton is in place"),
    array("commands",  0,   0,  "nothing done"),
    array("cli",       0,   0,  "nothing done"),
  );
  function getclass($p)
  {
    if ($p > 80)
      return "c";
    if ($p > 40)
      return "b";
    return "a";
  }
  foreach($status as $s)
  {
    $d = "<td class=\"".getclass($s[1])."\">${s[1]}</td>";
    $c = "<td class=\"".getclass($s[2])."\">${s[2]}</td>";
    echo "    <tr><td>${s[0]}</td>${d}${c}<td>${s[3]}</td></tr>\n";
  }
?>
  </table>

 <h1>Roadmap</h1>

  <p>Here is the current roadmap of the @SPKG@ development.</p>

  <table cellspacing="0">
    <tr><th>date</th><th>milestone</th></tr>
<?php
  $milestones = array(
    array(1,"idea of pkgtools C implementation", "2005-04-01"),
    array(1,"first code commited to the archive", "2005-04-20"),
    array(1,"500,000 files can be added to filedb per second on my 1GHz athlon", "2005-06-01"),
    array(1,"existing codebase stabilized", "2005-06-04"),
    array(1,"website created", "2005-06-09"),
    array(1,"legacydb entries parser optimized", "2005-06-12"),
    array(0,"filesystem transactions implemented", "2005-06-15"),
    array(0,"commands framework in place", "2005-06-19"),
    array(0,"three basic commands implemented", "2005-07-01"),
    array(0,"testing framework for commands finished", "2005-07-15"),
    array(0,"first fully functional release", "2005-08-01"),
    array(0,"more commands implemented", "2005-09-01"),
  );
  foreach($milestones as $m)
  {
    echo "    <tr class=\"".($m[0]?"p":"f")."\">".
         "<td>${m[2]}</td><td>${m[1]}</td></tr>\n";
  }
?>
  </table>

<?php foot(); ?>
