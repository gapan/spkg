<?php

if (file_exists("dl/releases/LATEST"))
{
  $st = stat("dl/releases/LATEST");
  $vf = file("dl/releases/LATEST");
  $version = str_replace("\n", "", $vf[0]);
  $reldate = date("Y-m-d H:i", $st["mtime"]);
}
else
{
  $version = "rc0";
  $reldate = "2006-07-21";
}

?>
<?php function head() { ?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
    "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html>
<head>
<title>Slackware Linux Package Manager (spkg)</title>

<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-2" />
<meta name="Keywords" content="spkg, pkgtools, package manager, slackware, linux, C, fast, implementation" />
<meta name="Description" content="Slackware Linux package manager (pkgtools) reimplementation. More than 10x faster than original pkgtools." />
<meta name="Author" content="Ondřej Jirman" />
<meta name="robots" content="all" />
<meta name="revisit-after" content="3 days" />
<meta name="shortcut icon" content="/favicon.png" />

<link rel="stylesheet" type="text/css" href="style.css" />

</head>
<body>

<div id="all">

 <h1 id="top">
  <span class="title">spkg</span> -
  <span class="subtitle">The Unofficial Slackware Linux Package Manager</span>
 </h1>

 <div id="topmenu">
  <a href="index.php">home</a> |
  <a href="dloads.php">downloads</a> |
  <a href="docs.php">documentation</a> |
  <a href="howto.php">howto</a> |
  <a href="history.php">history</a>
 </div>

 <div id="main">

<?php } ?>
<?php function foot() { global $reldate; ?>

 </div>

 <div id="botmenu">
 Written by Ondřej Jirman, 2005 - 2011<br/>
 Last update: <?php echo $reldate; ?><br/>
 Contact: <a href="mailto:megous@megous.com">Ondřej Jirman</a> (<a href="http://megous.com">megous.com</a>)<br/>
 </div>

</div>

<script type="text/javascript">

  var _gaq = _gaq || [];
  _gaq.push(['_setAccount', 'UA-4356236-3']);
  _gaq.push(['_trackPageview']);

  (function() {
    var ga = document.createElement('script'); ga.type = 'text/javascript'; ga.async = true;
    ga.src = ('https:' == document.location.protocol ? 'https://ssl' : 'http://www') + '.google-analytics.com/ga.js';
    var s = document.getElementsByTagName('script')[0]; s.parentNode.insertBefore(ga, s);
  })();

</script>

</body>
</html>
<?php } head(); ?>
