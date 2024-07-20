<?php 
$name  = "Ivy" . " " . "Hruli";
$price = 2 + 2 * 2;
?>

<!DOCTYPE html>
<html>
  <head>
    <title><?php echo "The Candy Store";?></title>
    <link rel="stylesheet" href="css/styles.css">
  </head>
  <body>
    <h1><?php // echo "The Candy2 Store"; echo "The Candy3 Store";
    echo "The Candy5 StoreX";
    ?></h1>
    <h2>Shopping Cart</h2>
    <p>Items: <?php /* echo "Candy2 " . $name; */ echo "The Candy4 Store"; 
    echo "The Candy6 Store";
    ?></p>
    <p>Cost per pack: $<?php echo "20";?></p>
    <p>$price: $<?php echo $price;?></p>
	<p>User from data base: <?php db "select users";?></p>
	<p>Name: <?php echo $name;?></p>
	<p>2 + 2 * 2: <?php echo 2 + 2 * 2;?></p>
	<p>(2 + 2) * 2: <?php echo (2 + 2) * 2;?></p>
	
	<p>EXAMPLE: 
	
	<?php

$var1 = 10;
$var2 = 20;
$result = 8;

$result = $var1 + $var2; 
$result = $var1 - $var2; 
$result = $var1 * $var2; 
$result = $var1 / $var2; 

$result2 = $var1 + $var2;
?>

<p>String: <?php echo "Hello, World!";?></p>
<p>String: <?php echo "Hello," . " World! concat";?></p>
<p>$var1: <?php echo $var1;?></p>
<p>$var1 + $var2: <?php echo $var1 + $var2;?></p>
<p>$result2: <?php echo $result2;?></p>






<?php
$sum = $var1 + $var2; 
$difference = $var1 - $var2; 
$product = $var1 * $var2; 
$quotient = $var1 / $var2; 
?>

<p>$sum: <?php echo $sum;?></p>
<p>$difference: <?php echo $difference;?></p>
<p>$product: <?php echo $product;?></p>
<p>$quotient: <?php echo $quotient;?></p>



	
	
	</p>
	

  </body>
</html>
