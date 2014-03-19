<?php

/**
 * @file
 * Drush integration of commerce_drush.
 *
 * commerce-order-add - Create an order
 */

/**
 * Implement hook_drush_help().
 */
function commerce_drush_drush_help($section) {
  switch ($section) {
    case 'drush:commerce-order-add':
      $help = dt('Create an order.');
      return $help;
  }
}

/**
 * Implement hook_drush_command().
 */
function commerce_drush_drush_command() {
  $items = array();

  $items['commerce-order-add'] = array(
    'callback' => 'commerce_drush_drush_place_order',
    'drupal dependencies' => array('commerce_drush'),
    'description' => 'Create an order.',
    'bootstrap' => DRUSH_BOOTSTRAP_MAX, // MAX is required for --user option to be used.
    'aliases' => array('coa'),
    'arguments' => array(
      'products' => 'A space delimited list of product IDs or SKUs.',
    ),
    'options' => array(
      'quantity' => 'Total quantity of product.',
      ),
    'examples' => array(
      'drush coa 1' => 'Create an order for product 1.',
      'drush coa SKU1001 --quantity=2' => 'Create an order for 2 products of SKU SKU1001.',
    ),
  );

  return $items;
}

/**
 * Place a new order via drush
 */
function commerce_drush_drush_place_order() {
  global $user;

  // The provided products specified in the command.
  $products = _convert_csv_to_array(func_get_args());

  // Create the new order in checkout; you might also check first to
  // see if your user already has an order to use instead of a new one.
  $order = commerce_order_new($user->uid, 'checkout_checkout');

  // Save the order to get its ID.
  commerce_order_save($order);

  drush_log('Order created. Commerce order id is now ' . $order->order_id);

  foreach ($products as $product_id) {

    // Add the product to the cart

    // Load whatever product represents the item the customer will be paying for and create a line item for it.
    $product = is_numeric($product_id) ? commerce_product_load((int)$product_id) : commerce_product_load_by_sku($product_id);

    $quantity = max(current(drush_get_option_list('quantity')), 1);

    drush_print('Saving ' . $quantity . ' item(s) of product ' . $product_id . ' into new order...');

    if ((empty($product->product_id)) || (!$product->status)){
      if ($is_cli) {
        drush_log(dt('Cannot match given product id with a Commerce product ID/SKU.'));
      }

      drush_log(dt('Invalid product ID/SKU: @id', array('@id' => $product_id)));
      return FALSE;
    }

    // Create new line item based on selected product
    $line_item = commerce_product_line_item_new($product, 1, $order->order_id);

    // Save the line item to get its ID.
    commerce_line_item_save($line_item);
  } // end: foreach

  // Add the line item to the order by using wrapper.
  $order_wrapper = entity_metadata_wrapper('commerce_order', $order);
  $order_wrapper->commerce_line_items[] = $line_item;

  // Save the order again to update its line item reference field.
  commerce_order_save($order);

  drush_print(dt('Order @id created (See: /admin/commerce/orders/@id).', array('@id' => $order->order_id)));
}

