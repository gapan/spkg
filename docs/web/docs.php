<?php require "inc/common.php"; ?>
<?php head("spkg - documentation page"); ?>

 <h1>Documentation</h1>

  <p>Documentation for @SPKG@ is included in the source package, but you
  can also <a href="/dl/spkg-docs.tar.gz">download</a> it
  separately.</p>

 <h1>Online Documentation</h1>

  <p>Online documentation is avalibale <a href="/dl/spkg-docs/index.html">here</a>.</p>

 <h1>My own release terminology</h1>
 
  <p>Nothing revolutionary here. Just to explain what <strong>I</strong> mean
  by these words:</p>

  <dl>
   <dt>alpha</dt>
    <dd>Release is not fully featured, but implemented features
    work and has no known bugs. Typically not all planed commands
    will be implemented in these releases.</dd>

   <dt>beta</dt>
    <dd>All planed features are implemented, but software has not
    gone through widespread testing yet. There may still be minor
    changes to the not so widely used api.</dd>

   <dt>rc</dt>
    <dd>Total feature freeze. Only bugs can be fixed and documentation
    can be improved during this phase. Everything seems working fine.</dd>

   <dt>1.0</dt>
    <dd>End of story.</dd>
  </dl>

<?php foot(); ?>
