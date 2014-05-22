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
    case 'drush:generate-customer-profiles':
      return dt('Usage: drush generate-customer-profiles <entity_type>.');
    case 'drush:commerce-order-pay':
      return dt('Usage: drush --user=USER_NAME_OR_UID commerce-order-pay');
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
      'products' => dt('A space delimited list of product IDs or SKUs.'),
    ),
    'required-arguments' => TRUE,
    'options' => array(
      'quantity' => 'Total quantity of product.',
      'empty' => 'Empty current shopping cart order before placing the order.',
      ),
    'examples' => array(
      'drush commerce-order-add 1' => 'Create an order for product 1.',
      'drush coa 2,3' => 'Create an order for product 2 and 3.',
      'drush --user=admin coa SKU1001 --quantity=2 --timediff="-3 year"' => 'Create an order for 2 products with SKU SKU1001.',
    ),
    'core' => array(7),
  );

  $items['commerce-order-pay'] = array(
    'callback' => 'commerce_drush_drush_order_pay',
    'drupal dependencies' => array('commerce_drush', 'commerce_payment_example'),
    'description' => 'Creates full payment for the (cart) order and completes the order.',
    'bootstrap' => DRUSH_BOOTSTRAP_MAX, // MAX is required for --user option to be used.
    'aliases' => array('cop'),
    'arguments' => array(
    ),
    'required-arguments' => TRUE,
    'options' => array(
      ),
    'examples' => array(
      'drush --user=admin commerce-order-pay' => 'Completes (cart) order for the admin user',
    ),
    'core' => array(7),
  );

  $items['generate-customer-profiles'] = array(
    'description' => 'Creates dummy customer profile data.',
    'arguments' => array(
      'bundle_types' => 'The type of entity to generate customer profiles for (e.g. billing).',
      'number' => 'Number of customer profiles to generate. Defaults to 1.',
    ),
    'required-arguments' => TRUE,
    'drupal dependencies' => array('devel_generate'),
    'options' => array(
      'kill' => 'Specify \'kill\' to delete all existing customer profiles before generating new ones.',
      'users' => 'Comma-delimited list of users to operate on.',
      'node_types' => 'A comma delimited list of node types to generate customer profiles for, if the entity type is \'node\'.',
    ),
    'examples' => array(
      'drush generate-customer-profiles billing 1' => 'Generates customer profiles for all users.',
      'drush --kill --users=admin generate-customer-profiles billing 1' => "Replaces all existing admin's customer profiles with a new one.",
      'drush --force generate-customer-profiles billing 1' => 'Generates customer profiles for all users and skips the prompt for continuation when there are more that 20 users in the database.',
      'drush --users admin --entity-type="my-customer-profile-entity-type" billing 1' => 'Generates a single customer profile for admin using specified custom entity type.',
      'drush --user admin --users admin billing 1' => 'Create Billing profile for admin user being logged as admin.',
    ),
    'aliases' => array('gencp'),
  );

  return $items;
}

/**
 * Place a new order via drush
 */
function commerce_drush_drush_place_order() {
  if (!is_object($user = drush_get_option('user'))) {
    if (is_numeric($user)) {
      $user = user_load($user);
    }
    else {
      $user = user_load_by_name($user);
    }
  }

  // The provided products specified in the command.
  $products = _convert_csv_to_array(func_get_args());

  if ($products) {

    // Create the new order in checkout; you might also check first to
    // see if your user already has an order to use instead of a new one.

    $order = commerce_cart_order_load($user->uid);

    if ($order === FALSE) {
      try {
        $order = commerce_cart_order_new($user->uid);
      } catch (DatabaseTransactionOutOfOrderException $e) {
        // It's a bug, when inserting a new order, the
        // commerce_addressbook_commerce_order_insert hook tries to save it
        // again and thus it's breaking transaction order.
      }
    }

    drush_log('Order created. Commerce order id is now ' . $order->order_id);

    if (($time_diff = drush_get_option('timediff')) !== NULL) {
      $order->created
        = $order->updated = strtotime($time_diff, REQUEST_TIME);
    }

    if ($empty = drush_get_option_list('empty')) {
      // Deletes every product line item from a shopping cart order.
      commerce_cart_order_empty($order);
    }

    foreach ($products as $product_id) {

      // Add the product to the cart

      // Load whatever product represents the item the customer will be paying for and create a line item for it.
      $product = is_numeric($product_id) ? commerce_product_load((int)$product_id) : commerce_product_load_by_sku($product_id);

      $quantity = max(current(drush_get_option_list('quantity')), 1);

      drush_print('Saving ' . $quantity . ' item(s) of product ' . $product_id . ' into new order...');

      if ((empty($product->product_id)) || (!$product->status)){
        drush_log(dt('Cannot match given product id with a Commerce product ID/SKU.'));

        drush_log(dt('Invalid product ID/SKU: @id', array('@id' => $product_id)));
        return FALSE;
      }

      // Create new line item based on selected product
      $line_item = commerce_product_line_item_new($product, 1, $order->order_id);

      // Adding line item to the cart.
      commerce_cart_product_add($user->uid, $line_item);
    } // end: foreach

//    // Add the line item to the order by using wrapper.
//    $order_wrapper = entity_metadata_wrapper('commerce_order', $order);
//    $order_wrapper->commerce_line_items[] = $line_item;

    // Save the order again to update its line item reference field.
    commerce_order_save($order);

    drush_print(dt('Order @id created (See: /admin/commerce/orders/@id).', array('@id' => $order->order_id)));

  } else {  // The user did not specify any products in the command
    // TODO: We can implement here prompting user for a product to add. Remove drush required-arguments to proceed.

/*
    // list of choices for the user
    // $products = GET PRODUCTS
    $choice = drush_choice($products, 'Enter a product to order.', '!key'); // prompt the user

    if ($choice !== FALSE) {
      // revert all views option
      // Add product to the cart
      $i++;
    }
*/
  }

}

/**
 * Command callback. Generate a number of votes.
 */
function drush_commerce_drush_generate_customer_profiles($bundle_types, $number = 1) {
  $options = array(
    'entity_type' => 'commerce_customer_profile',
    'kill' => drush_get_option('kill'),
    'types' => explode(',', $bundle_types),
    'users' => explode(',', drush_get_option('users') ?: ''),
    'number' => $number,
  );

  if (!empty($options['users'])) {
    // We will generate customer profile only for the specified user(s).
    $uids = array();

    foreach ($options['users'] as $name) {
      if (is_numeric($name)) {
        $uids[] = $name;
      }
      else {
        $uids[] = user_load_by_name($name)->uid;
      }
    }
  }
  else {
    // We will generate customer profiles for all users.
    module_load_include('inc', 'devel_generate');

    // We will iterate over all users.
    $uids
      = array_values(
          db_query("SELECT uid FROM {users}")
            ->fetchAllKeyed()
        );


    if (count($uids) > 20 && !drush_get_option('force')) {
      $answer = drush_choice(array('y' => 'Continue'), dt("There's " . count($uids) . " users in the database. That operation could take some time to complete. Do you want to continue?"));

      if ($answer !== 'y') {
        // User don't want to continue, exitting.
        return;
      }
    }
  }

  // We will pass UIDs into the commerce_drush_generate_customer_profiles's
  // options to speed up things.
  $options['users'] = $uids;

  drush_log(t('Generating [@btype] customer profiles for @etype entities.', array(
    '@btype' => implode(', ', $options['types']),
    '@etype' => $options['entity_type'])
  ), 'success');

  commerce_drush_generate_customer_profiles($options);
}

/**
 * Utility function to generate votes.
 */
function commerce_drush_generate_customer_profiles($options = array()) {

  // Applying default options.
  $options += array(
    'entity_type' => 'commerce_customer_profile',
    'bundle_types' => array('billing'),
    'kill' => FALSE,
    'types' => '',
    'users' => array(),
    'number' => 1,
  );

  $uids = $options['users'];

  // Converting names to UIDs if neccessary.
  foreach ($uids as &$uid) {
    if (!is_numeric($uid)) {
      $uid = user_load_by_name($uid)->uid;
    }
  }

  if (!$entity_type) {
    $entity_type = 'commerce_customer_profile';
  }

  $schema = drupal_get_schema($entity_type);

  if (!$schema) {
    throw new Exception ("There is no entity type with name \"$entity_type\"");
  }

  $entity_id_column = array_shift($schema['primary key']);

  if (empty($entity_id_column)) {
    // We'll use default entity key, the "entity_id".
    $entity_id_column = 'entity_id';
  }

  $transaction = db_transaction();

  try {

    if (drush_get_option('kill')) {
      // Removing existing customer profiles.
      // Retrieving commerce customer profiles for the all UIDs.
      $query
        = db_select($entity_type, 'e')
          ->fields('e', array($entity_id_column));

      if ($entity_type == 'commerce_customer_profile' && !empty($options['types'])) {
        $query->condition('e.type', $options['types'], 'IN');
        $query->condition('e.uid', $uids, 'IN');
      }

      $results = $query
        ->execute()
        ->fetchAll(PDO::FETCH_ASSOC);

      foreach ($results as $entity_id) {
        //entity_delete($entity_type, $entity_id);
      }
    }

    module_load_include('inc', 'devel_generate', 'devel_generate');
    module_load_include('inc', 'devel_generate', 'devel_generate.fields');

    // Creating new customer profiles.
    foreach ($uids as $uid) {
      $customer_user = user_load($uid);

      foreach ($options['types'] as $bundle_type) {
        for ($i = 0; $i < $options['number']; ++$i) {

          $customer_profile = commerce_customer_profile_new($bundle_type, $uid);

          devel_generate_fields($customer_profile, $entity_type, $bundle_type);

          if (module_exists('commerce_addressbook')) {
            if (isset($customer_profile->commerce_customer_address)) {
              $address = &$customer_profile->commerce_customer_address[LANGUAGE_NONE][0];

              $address['name_line']          = $customer_user->name . ' - prof';
              $address['first_name']         = $customer_user->name;
              $address['last_name']          = devel_generate_word('12');
              $address['organisation_name']  = $customer_user->name . ' - org';
              $address['premise']            = devel_generate_word('14');
            }
          }

          commerce_customer_profile_save($customer_profile);
        }
      }
    }
  }
  catch (Exception $e) {
    $transaction->rollback();
    watchdog_exception('Commerce Drush', $e);
  }
}

function commerce_drush_drush_order_pay() {
  if (!is_object($user = drush_get_option('user'))) {
    if (is_numeric($user)) {
      $user = user_load($user);
    }
    else {
      $user = user_load_by_name($user);
    }
  }
 
  $payment_method = commerce_payment_method_instance_load('commerce_payment_example|commerce_payment_commerce_payment_example');

  $order = commerce_cart_order_load($user->uid);

  if (!$order) {
    drush_set_error('Commerce Drush', dt('Specified user has no cart order'));
    return;
  }
  
  commerce_order_status_update($order, 'checkout_review');

    // Creating new transaction via commerce_payment_example module.
  $charge = $order->commerce_order_total['und'][0];

  $transaction = commerce_payment_transaction_new('commerce_payment_example', $order->order_id);
  $transaction->instance_id = $payment_method['instance_id'];
  $transaction->amount = $charge['amount'];
  $transaction->currency_code = $charge['currency_code'];
  $transaction->status = COMMERCE_PAYMENT_STATUS_SUCCESS;
  $transaction->message = 'Name: @name';
  $transaction->message_variables = array('@name' => 'Example payment');

  commerce_payment_transaction_save($transaction);
  commerce_payment_commerce_payment_transaction_insert($transaction);
  commerce_checkout_complete($order);
}
