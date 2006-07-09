<?php function head($title) { ?>
<!DOCTYPE html PUBLIC "-//W3C//DTD XHTML 1.0 Transitional//EN"
    "http://www.w3.org/TR/xhtml1/DTD/xhtml1-transitional.dtd">
<html>
<head>
<title><?php echo $title; ?></title>

<meta http-equiv="Content-Type" content="text/html; charset=iso-8859-2" />
<meta name="Keywords" content="spkg, speedy package manager, slackware, linux, C, fast, implementation" />
<meta name="Description" content="spkg - The Unofficial Slackware Linux Package Manager" />
<meta name="Author" content="Ondřej Jirman" />
<meta name="robots" content="all" />
<meta name="revisit-after" content="3 days" />
<meta name="shortcut icon" content="/favicon.png" />

<link rel="stylesheet" type="text/css" href="/style.css" />

</head>
<body>

<div id="all">

 <div id="top">
  <div class="subtitle">The Unofficial Slackware Linux Package Manager</div>
  <div class="title"><span style="color:red;">s</span>pkg</div>
 </div>

 <div id="topmenu">
  <a href="index.php">home</a> |
  <a href="dloads.php">downloads</a> |
  <a href="status.php">status</a> |
  <a href="docs.php">documentation</a> |
  <a href="howto.php">howto</a> |
  <a href="history.php">history</a>
 </div>

 <div id="main">

<?php } ?>
<?php function foot() { ?>

 </div>

 <div id="botmenu">
 Written by Ondřej Jirman, 2005-2006
 | last update: @DATE@ 
 | contact: <a href="mailto:megous@megous.com">megous@megous.com</a>
 | <a href="http://megous.com">megous.com</a>
 </div>

</div>
</body>
</html>
<?php } ?>
