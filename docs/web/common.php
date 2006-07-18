<?php function head($title) { ?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
    "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html>
<head>
<title><?php echo $title; ?></title>

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
  <span class="title">spkg</span>
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
<?php function foot() { ?>

 </div>

 <div id="botmenu">
 Written by Ondřej Jirman, 2005 - 2006<br/>
 Last update: @DATE@<br/>
 Contact: <a href="mailto:megous@megous.com">Ondřej Jirman</a> (<a href="http://megous.com">megous.com</a>)<br/>
 </div>
 <div id="lonmenu">
 Listed on:
 <a href="http://freshmeat.net"><img src="img/freshmeat.gif" alt="Freshmeat.net" /></a>
 <a href="http://linuxlinks.com"><img src="img/linuxlinks.gif" alt="Linuxlinks.com" /></a>
 <a href="http://softpedia.com">Softpedia.com</a>
 </div>

</div>
</body>
</html>
<?php } ?>
