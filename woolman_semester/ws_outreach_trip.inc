<?php

civicrm_initialize();

/**
 * Multi-page form.
 */
function ws_outreach_form($form_state) {

  drupal_add_css(drupal_get_path('module', 'woolman_semester') . '/css/ws_admissions.css', 'module', 'all', FALSE);
  drupal_add_js(drupal_get_path('module', 'woolman_semester') .'/js/ws_admissions.js', 'module', 'header');

   // Initialize form page counter.
  if(!isset($form_state['storage']['page'])) $form_state['storage']['page'] = 1;
  $form = array();

  switch($form_state['storage']['page']) {

  // Page one of the form.
  case 1:

    // get option values from Civi DB
    $resource = db_query("SELECT label_a_b, id FROM {civicrm_relationship_type} WHERE contact_type_a='Individual' AND  contact_type_b='Organization' ");
    $relationship_types = array(0 => t('- none -'));
    while ($row = db_fetch_array($resource)) {
      $relationship_types[$row['id']] = $row['label_a_b'];
    }
    $resource = db_query("SELECT label,name FROM {civicrm_contact_type} WHERE parent_id=3 ORDER BY label");
    $org_types = array('none' => 'Organization');
    while ($row = db_fetch_array($resource)) {
      $org_types[$row['name']] = $row['label'];
    }

    // Get cid & name of current user
    $self = woolman_contact_get();
    $default_user = $self['nick_name'] . ' ' . $self['last_name'] .  ' (ID#' . $self['contact_id'].')';


    // build form
    $form['#prefix'] = '<div class="page-one">';
    $form['organization'] = array(
      '#type' => 'fieldset',
      '#title' => t('Organization Visited'),
    );
    $form['organization']['organization_name'] = array(
      '#type' => 'textfield',
      '#title' => t('Organization Name'),
      '#size' => 50,
      '#description' => t('Pick an org from the database, or add a new one.'),
      '#required' => TRUE,
      '#autocomplete_path' => 'staff/js/autocomplete/Organization',
    );
    $form['organization']['organization_type'] = array(
    '#type' => 'select',
    '#title' => t('Organization Type'),
    '#options' => $org_types,
    );
    $form['organization']['organization_email'] = array(
      '#prefix' => '<div class="subset">',
      '#type' => 'textfield',
      '#title' => t('Email for organization'),
      '#size' => 20,
    );
    $form['organization']['organization_phone'] = array(
      '#type' => 'textfield',
      '#title' => t('Phone number'),
      '#size' => 20,
    );
    $form['organization']['organization_website'] = array(
      '#type' => 'textfield',
      '#title' => 'Website',
      '#size' => 20,
      '#suffix' => '</div>',
    );
    $form['organization']['organization_address'] = array(
      '#type' => 'textfield',
      '#title' => t('Street Address'),
    );
    $form['organization']['organization_city'] = array(
      '#prefix' => '<div class="subset">',
      '#type' => 'textfield',
      '#title' => t('City'),
      '#size' => 15,
    );
    $form['organization']['organization_state'] = array(
      '#type' => 'textfield',
      '#title' => t('State'),
      '#size' => 16,
      '#maxlength' => 2,
      '#autocomplete_path' => 'staff/js/autocomplete/state',
    );
    $form['organization']['organization_zip'] = array(
      '#type' => 'textfield',
      '#title' => t('Zip'),
      '#size' => 10,
      '#suffix' => '</div>',
    );
    $form['contact_person'] = array(
      '#type' => 'fieldset',
      '#title' => t('Contact Person (optional)'),
    );
    $form['contact_person']['contact_name'] = array(
      '#type' => 'textfield',
      '#title' => 'Name',
      '#description' => t('Person at above organization who helped you with your visit.'),
      '#size' => 50,
      '#autocomplete_path' => 'staff/js/autocomplete/Individual',
    );
    $form['contact_person']['contact_last'] = array(
      '#type' => 'textfield',
      '#title' => 'Last Name',
      '#size' => 30,
    );
    $form['contact_person']['contact_email'] = array(
      '#prefix' => '<div class="subset">',
      '#type' => 'textfield',
      '#title' => 'Email',
      '#size' => 20,
    );
    $form['contact_person']['contact_phone'] = array(
      '#type' => 'textfield',
      '#title' => 'Work phone',
      '#size' => 20,
    );
    $form['contact_person']['contact_cell'] = array(
      '#type' => 'textfield',
      '#title' => 'Cell phone',
      '#size' => 20,
      '#suffix' => '</div>',
    );
    $form['contact_person']['contact_to_newsletter'] = array(
    '#type' => 'checkbox',
    '#title' => t('Subscribe to newsletter'),
    '#default_value' => TRUE,
    );
    $form['contact_person']['contact_relationship'] = array(
    '#type' => 'select',
    '#title' => t('Relationship to above organization'),
    '#options' => $relationship_types,
    '#default_value' => 4,
    );
    $form['contact_person']['contact_to_case'] = array(
    '#type' => 'checkbox',
    '#title' => t('Add to each prospective student case'),
    '#default_value' => TRUE,
    );

    $form['details'] = array(
      '#type' => 'fieldset',
      '#title' => t('Trip Details'),
    );
    $form['details']['date'] = array(
      '#type' => 'date',
      '#title' => t('Date of Trip'),
      '#required' => TRUE,
    );
    $form['details']['outreach_person'] = array(
      '#type' => 'textfield',
      '#title' => t('Outreach Person'),
      '#description' => t('Admissions person who went on this trip. (I.E. You)'),
      '#default_value' => $default_user,
      '#size' => 30,
      '#autocomplete_path' => 'staff/js/autocomplete/Individual',
      '#required' => TRUE,
    );
    $form['details']['home'] = array('#prefix' => '<div id=homestay-wrapper>', '#suffix' => '</div>');
    $form['details']['home']['homebody_name'] = array(
      '#type' => 'textfield',
      '#title' => 'Person you stayed with',
      '#description' => 'Person who let you spend the night at their house (NOTE: don\'t enter the same person multiple times for the same trip).',
      '#size' => 30,
      '#autocomplete_path' => 'staff/js/autocomplete/Individual',
    );
    $form['details']['home']['homebody_last'] = array(
      '#type' => 'textfield',
      '#title' => 'Last',
      '#description' => 'Last Name',
      '#size' => 30,
    );
    $form['details']['home']['homebody_phone'] = array(
      '#prefix' => '<div class="subset">',
      '#type' => 'textfield',
      '#title' => 'Home Phone',
      '#size' => 15,
    );
    $form['details']['home']['homebody_cell'] = array(
      '#type' => 'textfield',
      '#title' => 'Cell Phone',
      '#size' => 15,
    );
    $form['details']['home']['homebody_email'] = array(
    '#type' => 'textfield',
    '#title' => t('Email'),
    '#size' => 20,
    );
    $form['details']['home']['homebody_to_newsletter'] = array(
    '#prefix' => '<br />',
    '#type' => 'checkbox',
    '#title' => t('Subscribe to newsletter'),
    '#default_value' => $form_state['storage']['newsletter_default'],
    '#suffix' => '</div>',
    );
    $form['details']['home']['homebody_address'] = array(
      '#type' => 'textfield',
      '#title' => t('Street Address'),
      '#size' => 45,
    );
    $form['details']['home']['homebody_city'] = array(
      '#prefix' => '<div class="subset">',
      '#type' => 'textfield',
      '#title' => t('City'),
      '#default_value' => $form_state['storage']['default_city'],
      '#size' => 14,
    );
    $form['details']['home']['homebody_state'] = array(
      '#type' => 'textfield',
      '#title' => t('State'),
      '#default_value' => $form_state['storage']['default_st'],
      '#size' => 14,
      '#maxlength' => 2,
      '#autocomplete_path' => 'staff/js/autocomplete/state',
    );
    $form['details']['home']['homebody_zip'] = array(
      '#type' => 'textfield',
      '#title' => t('Zip'),
      '#default_value' => $form_state['storage']['default_zip'],
      '#size' => 8,
      '#suffix' => '</div>',
    );
    $form['details']['create_activity'] = array(
    '#type' => 'checkbox',
    '#title' => 'Create activity',
    '#description' => 'Automatically create activity record for this trip',
    '#default_value' => TRUE,
    );
    $form['details']['activity_notes'] = array(
        '#type' => 'textarea',
        '#title' => 'Notes',
        '#rows' => 3,
      );

    $form['set_defaults'] = array(
      '#type' => 'fieldset',
      '#title' => 'Default settings for next step',
      '#description' => t('For convenience, you can choose whether checkboxes start off "checked" or "unchecked" on the next page, and if you want address fields pre-filled for you.'),
    );
    $form['set_defaults']['number_of_students'] = array(
      '#type' => 'textfield',
      '#title' => 'Number of contacts to enter',
      '#size' => 1,
      '#maxlength' => 2,
      '#default_value' => 10,
      '#description' => 'The next page will start off with this many blanks.',
      '#required' => TRUE,
    );
    $form['set_defaults']['newsletter_default'] = array(
    '#prefix' => '<div class="subset">',
    '#type' => 'checkbox',
    '#title' => 'Subscribe to newsletter',
    '#default_value' => TRUE,
    );
    $form['set_defaults']['case_default'] = array(
    '#type' => 'checkbox',
    '#title' => 'Create new case',
    '#default_value' => TRUE,
    '#suffix' => '</div>',
    );
    $form['set_defaults']['relationship_default'] = array(
    '#type' => 'select',
    '#title' => 'Default relationship to organization',
    '#description' => 'Choose what relationship most of these students have to above organization',
    '#options' => $relationship_types,
    '#default_value' => 10,
    );
    $form['set_defaults']['address_default'] = array(
    '#type' => 'checkbox',
    '#title' => 'Auto-fill city, state &amp; zip',
    '#description' => 'Set the default address for each student based on the address of the above organization',
    '#default_value' => TRUE,
    );
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => 'Enter Contacts >>',
    );
    $form['#validate'][] = 'ws_outreach_form_validate';

    break;

    case 2:
    // Page two: enter students.
    drupal_add_js(drupal_get_path('module', 'woolman_semester') .'/js/ws_admissions_2.js', 'module', 'header');

    // get option values from Civi DB
    $resource = db_query("SELECT label_a_b, id FROM {civicrm_relationship_type} WHERE contact_type_a='Individual' AND  contact_type_b='Organization' AND (contact_sub_type_b IS NULL OR contact_sub_type_b =' {$form_state['storage']['org']['sub_type']}')");
    $relationship_types = array(0 => t('- none -'));
    while ($row = db_fetch_array($resource)) {
      $relationship_types[$row['id']] = $row['label_a_b'];
    }

    //load semesters
    $semesters = woolman_get_civi_options(80);

    //grad date options
    $grad_dates = array(0 => 'unknown');
    $y = date('Y'); $y -= 2;
    for ($i=$y; $i<=$y+6; $i++) {
      $grad_dates[$i] = $i;
    }

    $form = array('#prefix' => '<div class="page-two">');
    // build requested number of fieldsets
    for ($i=1; $i<=$form_state['storage']['number_of_students']; $i++) {
      $form['student_0' . $i] = array(
      '#type' => 'fieldset',
      '#title' => "Contact $i",
      );
      $form['student_0' . $i]['first_student_0' . $i] = array(
      '#prefix' => '<div class="subset">',
      '#type' => 'textfield',
      '#title' => 'First name',
      '#size' => 12,
      );
      $form['student_0' . $i]['nick_student_0' . $i] = array(
      '#type' => 'textfield',
      '#title' => 'Nickname',
      '#size' => 10,
      );
      $form['student_0' . $i]['last_student_0' . $i] = array(
      '#type' => 'textfield',
      '#title' => t('Last name'),
      '#size' => 15,
      '#suffix' => '</div>',
      );
      $form['student_0' . $i]['email_student_0' . $i] = array(
      '#prefix' => '<div class="subset">',
      '#type' => 'textfield',
      '#title' => t('Email'),
      '#size' => 20,
      );
      $form['student_0' . $i]['newsletter_student_0' . $i] = array(
      '#prefix' => '<br />',
      '#type' => 'checkbox',
      '#title' => t('Subscribe to newsletter'),
      '#default_value' => $form_state['storage']['newsletter_default'],
      '#suffix' => '</div>',
      );
      $form['student_0' . $i]['relationship_student_0' . $i] = array(
      '#prefix' => '<div class="subset">',
      '#type' => 'select',
      '#title' => 'Relationship to ' . $form_state['storage']['org']['name'],
      '#options' => $relationship_types,
      '#default_value' => $form_state['storage']['relationship_default'],
      );
      $form['student_0' . $i]['grad_student_0' . $i] = array(
        '#type' => 'select',
        '#title' => t('Graduation Date'),
        '#options' => $grad_dates,
        '#suffix' => '</div>',
      );
      $form['student_0' . $i]['semester_student_0' . $i] = array(
        '#type' => 'checkboxes',
        '#title' => t('Interested in applying for'),
        '#prefix' => '<div class="subset">',
        '#suffix' => '</div>',
        '#options' => $semesters
      );
      $form['student_0' . $i]['address_student_0' . $i] = array(
        '#type' => 'textfield',
        '#title' => t('Street Address'),
        '#size' => 45,
      );
      $form['student_0' . $i]['city_student_0' . $i] = array(
        '#prefix' => '<div class="subset">',
        '#type' => 'textfield',
        '#title' => t('City'),
        '#default_value' => $form_state['storage']['default_city'],
        '#size' => 14,
      );
      $form['student_0' . $i]['state_student_0' . $i] = array(
        '#type' => 'textfield',
        '#title' => t('State'),
        '#default_value' => $form_state['storage']['default_st'],
        '#size' => 14,
        '#maxlength' => 2,
        '#autocomplete_path' => 'staff/js/autocomplete/state',
      );
      $form['student_0' . $i]['zip_student_0' . $i] = array(
        '#type' => 'textfield',
        '#title' => t('Zip'),
        '#default_value' => $form_state['storage']['default_zip'],
        '#size' => 8,
        '#suffix' => '</div>',
      );
      $form['student_0' . $i]['phone_student_0' . $i] = array(
        '#prefix' => '<div class="subset">',
        '#type' => 'textfield',
        '#title' => t('Phone Number'),
        '#size' => 15,
      );
      $form['student_0' . $i]['phone_location_student_0' . $i] = array(
        '#type' => 'select',
        '#title' => 'Type',
        '#options' => array(1 => 'Home Phone', 4 => 'Cell Phone', 2 => 'Work Phone'),
      );
      $form['student_0' . $i]['case_student_0' . $i] = array(
        '#prefix' => '<br />',
        '#type' => 'checkbox',
        '#title' => 'Prospective Student?',
        '#default_value' => $form_state['storage']['case_default'],
        '#suffix' => '</div>',
      );
      $form['student_0' . $i]['notes_student_0' . $i] = array(
        '#type' => 'textarea',
        '#title' => 'Case Notes',
        '#rows' => 3,
        '#cols' => 55,
        '#description' => 'Stored in the "open case" activity',
        '#resizable' => FALSE,
      );
    }
    $form['submit'] = array(
      '#type' => 'submit',
      '#value' => t('Done'),
    );
  }
  $form['#suffix'] = '</div>';
  return $form;
}

function ws_outreach_form_submit($form, &$form_state) {

  switch ($form_state['storage']['page']) {

  case 1:
    // Process page one submission
    $values = $form_state['values'];
    $form_state['rebuild'] = TRUE;
    $form_state['storage'] = $values + array('page' => 1);

    // process org submission
    $org = woolman_split_name_id($values['organization_name']);

    // org already in civi db
    if ($org['id']) {

      $retrieved = woolman_contact_get($org['id']);
      $org['sub_type'] = $retrieved['contact_sub_type'];
      $default_city = $retrieved['city'];
      $default_st = $retrieved['state_province'];
      $default_zip = $retrieved['postal_code'];
      $values['organization_address'] = $retrieved['street_address'];

      // status report to user
      if (!$default_city || !$default_st || !$default_zip) {
        $info = ($default_city || $default_st || $default_zip) ? 'incomplete' : 'no';
        drupal_set_message('There is '.$info.' address information for <em>'.$org['name'].'</em> in the CiviCRM database.<br />You may wish to <a target="_blank" href="/civicrm/contact/add?reset=1&action=update&cid='.$org['id'].'">update the record in CiviCRM</a>.', $type = 'warning');
      }
    }

    else {
      $default_city = $values['organization_city'];
      $default_st = $values['organization_state'];
      $default_zip = $values['organization_zip'];

      // create new org
      $org_params = array(
        'contact' => array(
          'organization_name' => $org['name'],
          'contact_type' => 'Organization',
          'source' => 'Outreach trip',
        ),
        'email' => array(array(
          'email' => $values['organization_email'],
          'location_type_id' => 3,
          'is_primary' => 1,
        )),
        'website' => array(array(
          'url' => $values['organization_website'],
          'is_primary' => 1,
        )),
        'address' => array(array(
          'is_primary' => 1,
          'street_address' => $values['organization_address'],
          'city' => $default_city,
          'postal_code' => $default_zip,
          'state_province' => $default_st,
          'country_id' => USA,
        )),
        'phone' => array(array(
          'phone' => $values['organization_phone'],
          'phone_type_id' => 1,
          'location_type_id' => 3,
          'is_primary' => 1,
        )),
      );
      if($values['organization_type'] != 'none') {
        $org_params['contact']['contact_sub_type'] = $values['organization_type'];
      }

      $result = woolman_contact_match($org_params);
      if (!empty($result['contact_id'])) {
        drupal_set_message('Created new record for '.$org['name'].'.');
        $org['id'] = $result['contact_id'];
      }
      else {
        drupal_set_message('Sorry, encountered an error while adding organization to the database. Please contact site administrator.', 'error');
      }
    }

    // process contact person
    if ($values['contact_name']) { //was anything entered for contact name?

      $contact = woolman_split_name_id($values['contact_name']);

      // add contact to db if not already there
      if (!$contact['id']) {
        $params = array(
          'contact' => array(
            'first_name' => $contact['name'],
            'last_name' => $values['contact_last'],
            'source' => 'Outreach trip to ' . $org['name'],
          ),
          'email' => array(array(
            'email' => $values['contact_email'],
            'is_primary' => 1,
          )),
        );
        if ($values['contact_phone'] || $values['contact_cell']) {
          $params['phone'] = array();
          if ($values['contact_phone']) {
            $params['phone'][] =  array(
              'phone' => $values['contact_phone'],
              'phone_type_id' => 1,
              'location_type_id' => 2,
              'is_primary' => $values['contact_cell'] == '',
            );
          }
          if ($values['contact_cell']) {
            $params['phone'][] =  array(
              'phone' => $values['contact_cell'],
              'phone_type_id' => 1,
              'location_type_id' => 4,
              'is_primary' => TRUE,
            );
          }
        }
        if ($values['contact_relationship'] == 4) {
          $params['address'] = array('share_with_contact' => $org['id']);
        }
        $result = woolman_contact_match($params);
        $contact['id'] = $result['contact_id'];
        $contact['name'] .= ' ' . trim($values['contact_last']);

        // status report to user
        if ($result['is_error']) {
          drupal_set_message('Sorry, unable to add '.$contact['name'].' to the database due to an error. Please contact site administrator.<br />Error message: '.$result['error_message'], 'error');
        }
        else {
          drupal_set_message('Created new record for '.$contact['name'].'.');

          // add to newsletter
          if ($values['contact_to_newsletter']) {

            $result = woolman_civicrm_api('group_contact', 'create', array('contact_id' => $contact['id'], 'group_id' => 2));

            // status report to user
            if (!($result['is_error'])) {
              drupal_set_message('Subscribed '.$contact['name'].' to the newsletter.');
            }
            else {
              drupal_set_message('Sorry, unable to subscribe '.$contact['name'].' to the newsletter due to an error. Please contact site administrator.<br />Error message: '.$result['error_message'], 'error');
            }
          } // end add to newsletter

          // create relationship
          if ($values['contact_relationship']) {
            $params = array(
              'contact_id_a' => $contact['id'],
              'contact_id_b' => $org['id'],
              'relationship_type_id' => $values['contact_relationship'],
              'is_active' => 1
            );
            $result = woolman_civicrm_api('relationship', 'create', $params);
          }
        }
      }
      $form_state['storage']['contact'] = $contact;
    }

    // process home-stay person
    if ($values['homebody_name']) {

      $homebody = woolman_split_name_id($values['homebody_name']);

      // add homebody to db if not already there
      if (empty($homebody['id'])) {
        $params = array(
          'contact' => array(
            'first_name' => $homebody['name'],
            'last_name' => $values['homebody_last'],
            'source' => 'Outreach trip to ' . $org['name'],
          ),
          'email' => array(array(
            'email' => $values['homebody_email'],
            'is_primary' => 1
          )),
          'address' => array(array(
            'is_primary' => 1,
            'street_address' => $values['homebody_address'],
            'city' => $values['homebody_city'],
            'postal_code' => $values['homebody_zip'],
            'state_province' => $values['homebody_state'],
            'country_id' => USA,
          )),
        );
        if ($values['homebody_phone'] || $values['homebody_cell']) {
          $params['phone'] = array();
          if ($values['homebody_phone']) {
            $params['phone'][] =  array(
              'phone' => $values['homebody_phone'],
              'phone_type_id' => 1,
              'location_type_id' => 1,
              'is_primary' => ($values['homebody_cell']=='') ,
            );
          }
          if ($values['homebody_cell']) {
            $params['phone'][] =  array(
              'phone' => $values['homebody_cell'],
              'phone_type_id' => 1,
              'location_type_id' => 4,
              'is_primary' => TRUE,
            );
          }
        }
        $result = woolman_contact_match($params);
        $homebody['id'] = $result['contact_id'];
        $homebody['name'] .= ' '.trim($values['homebody_last']);

        // status report to user
        if (!empty($result['is_error'])) {
          drupal_set_message('Sorry, unable to add '.$homebody['name'].' to the database due to an error. Please contact site administrator.<br />Error message: '.$result['error_message'], 'error');
        }
        else {
          drupal_set_message('Created new record for '.$homebody['name'].'.');

          // add to newsletter
          if ($values['homebody_to_newsletter']) {
            $result = woolman_civicrm_api('group_contact', 'create', array('contact_id' => $homebody['id'], 'group_id' => 2));

            // status report to user
            if (empty($result['is_error'])) {
              drupal_set_message('Subscribed '.$homebody['name'].' to the newsletter.');
            }
            else {
              drupal_set_message('Sorry, unable to subscribe '.$homebody['name'].' to the newsletter due to an error. Please contact site administrator.<br />Error message: '.$result['error_message'], 'error');
            }
          }
        }
      }
      // create activity
      $outreach_person = woolman_split_name_id($values['outreach_person']);
      $params = array(
        'activity_type_id' => 48,
        'source_contact_id' => $outreach_person['id'],
        'target_contact_id' => $homebody['id'],
        'subject' => $homebody['name'].' gave '.$outreach_person['name'].' a place to stay.',
        'status_id' => 2,
        'activity_date_time' => woolman_date_array_to_string($values['date']),
        'details' => $outreach_person['name'].' stayed with '.$homebody['name'].' while on an outreach trip to '.$org['name'],
      );
      $result = woolman_civicrm_api('activity', 'create', $params);

      // status report to user
      if ($result['is_error']) {
        drupal_set_message('Sorry, unable to create homestay activity with '.$homebody['name'].' due to an error. Please contact site administrator.<br />Error message: '.$result['error_message'], 'error');
      }
    } // end processing homebody person

    // set default address for next form, respecting user preferences
    if (($values['address_default'])) {
      $form_state['storage']['default_city'] = $default_city;
      $form_state['storage']['default_st'] = $default_st;
      $form_state['storage']['default_zip'] = $default_zip;
    }
    $form_state['storage']['org'] = $org;

  break;

  case 2:
    // Process page two submission
    $storage = $form_state['storage'];
    // use post data to get info from the on-the-fly fieldsets too
    $post = $form_state['clicked_button']['#post'];

    $outreach_person = woolman_split_name_id($storage['outreach_person']);

    // start counter for number of successes to report to user at end
    $success = array_fill_keys(array('create', 'group', 'case', 'case_contact'), 0);
    $errors = array();

    $date = woolman_date_array_to_string($storage['date']);

    // Loop for as long as #post data exists
    for ($i = 1; isset($post['first_student_0'.$i]); ++$i) {
      if (empty($post['first_student_0'.$i]) || empty($post['last_student_0'.$i])) {
        continue;
      }
      $name = $post['first_student_0'.$i].' '.$post['last_student_0'.$i];

      // add student to Civi DB
      $params = array(
        'contact' => array(
          'first_name' => $post['first_student_0'.$i],
          'last_name' => $post['last_student_0'.$i],
          'nick_name' => $post['nick_student_0'.$i],
          'source' => 'Outreach trip to ' . $storage['org']['name'],
        ),
        'email' => array(array(
          'email' => $post['email_student_0'.$i],
          'is_primary' => 1,
        )),
      );
      if ($post['grad_student_0'.$i]) {
        $params['contact']['custom_4'] = $post['grad_student_0'.$i].'0601';
      }
      if ($post['phone_student_0'.$i]) {
        $params['phone'] = array(array(
          'phone' => $post['phone_student_0'.$i],
          'phone_type_id' => 1,
          'location_type_id' => $post['phone_location_student_0'.$i],
          'is_primary' => 1,
        ));
      }
      if ($post['address_student_0'.$i] || $post['city_student_0'.$i] || $post['state_student_0'.$i] || $post['zip_student_0'.$i]) {
        $params['address'] = array(array(
          'location_type_id' => 1,
          'is_primary' => 1,
          'street_address' => $post['address_student_0'.$i],
          'city' => $post['city_student_0'.$i],
          'postal_code' => $post['zip_student_0'.$i],
          'state_province' => $post['state_student_0'.$i],
          'country_id' => USA,
        ));
      }
      $result = woolman_contact_match($params);

      if (!empty($result['contact_id'])) {
        $success['create']++;
        $id[$i] = $result['contact_id'];

        // process relationship
        if ($post['relationship_student_0'.$i]) {
          $params = array(
            'contact_id_a' => $id[$i],
            'contact_id_b' => $storage['org']['id'],
            'relationship_type_id' => $post['relationship_student_0'.$i],
            'is_active' => 1
          );
          if ($post['grad_student_0'.$i] && $post['relationship_student_0'.$i] == 10) {
            $params['end_date'] = $post['grad_student_0'.$i] . '0601';
          }
          $result = woolman_relationship_add_or_update($params);
        }

        // process group
        if ($post['email_student_0'.$i] && $post['newsletter_student_0'.$i]) {
          $result = woolman_civicrm_api('group_contact', 'create', array('contact_id' => $id[$i], 'group_id' => 2));
          ws_outreach_message($result, 'group', $name, $errors, $success);
        }

         // don't get fooled by old data when storing this contact
        $new_case = NULL;

        // Process case
        if($post['case_student_0'.$i]) {
          // See if the case already exists
          module_load_include('inc', 'woolman_semester', 'ws_apply_online_utils');
          if (!($case = _ws_apply_get_case($id[$i], 1))) {
            $params = array(
              'case_type_id' => 1,
              'start_date' => $date,
              'status_id' => 1,
              'case_type' => 'Prospective Student',
              'contact_id' => $id[$i],
              'creator_id' => $outreach_person['id'],
              'subject' => 'Outreach Trip to '.$storage['org']['name'],
              'location' => $storage['org']['name'],
              'start_date' => $date,
              'medium_id' => '1', //"In Person"
              'details' => nl2br($post['notes_student_0'.$i]),
            );
            // auto-fill details if none provided
            if (!$params['details']) {
              $params['details'] = '<p>'.$outreach_person['name'].' met '.$post['first_student_0'.$i].' during an outreach trip to '.$storage['org']['name'].'.</p>';
            }

            $new_case = woolman_civicrm_api('case', 'create', $params);
            $case = $new_case['values'];

            ws_outreach_message($new_case, 'case', $name, $errors, $success);

            // Add contact person to case
            if($storage['contact']['id'] && $storage['contact_to_case']) {

              $params = array(
                'contact_id_a' => $id[$i],
                'contact_id_b' => $storage['contact']['id'],
                'relationship_type_id' => 14,
                'is_active' => 1,
                'case_id' => $case['id'],
              );
              $result = woolman_civicrm_api('relationship', 'create', $params);

              ws_outreach_message($result, 'case_contact', $name, $errors, $success);
            }
          }

          // Add semester choice activity
          if($post['semester_student_0'.$i]) {
            $subject = implode(', ', array_intersect_key($form['student_0'.$i]['semester_student_0'.$i]['#options'], $post['semester_student_0'.$i]));
            $params = array(
              'case_id' => $case['id'],
              'activity_type_id' => 47,
              'source_contact_id' => $outreach_person['id'],
              'status_id' => 2,
              'medium_id' => 1,
              'subject' => $subject,
              'custom_5' => woolman_multivalue_str($post['semester_student_0'.$i], TRUE),
            );
            if (!empty($case['acts'][47])) {
              $params['id'] = $case['acts'][47][0]['id'];
            }
            $result = woolman_civicrm_api('activity', 'create', $params);
          }
        } // End processing case

      } // End if contact created sucessfully

      else { // Couldn't create contact, so everything else got skipped
        $errors['student'][] = '<li>'.$post['first_student_0'.$i].' '.$post['last_student_0'.$i].'; Error creating contact.</li>';
      }
    } // end student processing loop

    // create activity
    if($storage['create_activity'] && $success['create']) {

      $target_ids = $id;
      if ($storage['contact']['id']) array_unshift($target_ids, $storage['contact']['id']);
      array_unshift($target_ids, $storage['org']['id']);

      $activity = array(
        'activity_type_id' => 33,
        'source_contact_id' => $outreach_person['id'],
        'target_contact_id' => $target_ids,
        'subject' => 'Outreach Trip to '.$storage['org']['name'],
        'status_id' => 2,
        'activity_date_time' => $date,
        'details' => $outreach_person['name'].' met '.$success['create'].' students at '.$storage['org']['name'].'. '.$success['group'].' signed up for the newsletter.',
      );

      if (strlen($storage['activity_notes']) > 3)
        $activity['details'] = '<p>'.nl2br($storage['activity_notes']).'</p>';

      $result = woolman_civicrm_api('activity', 'create', $activity);

      if (!empty($result['is_error'])) {
        drupal_set_message('Sorry, unable to create activity record due to an error. Please contact site administrator.<br />Error message: '.$result['error_message'], 'error');
      }
      else {
        $success['activity'] = TRUE;
        $form_state['storage']['activity_id'] = $result['id'];
      }

    }
    // Status report to user
    if ($success['create']) {
      drupal_set_message('Added '.$success['create'].' students to the database.');
      drupal_set_message('Created '.$success['relationship'].' relationships to '.$storage['org']['name'].', and added '.$success['group'].' newsletter subscriptions.');
      if ($success['case']) {
        if ($success['case_contact']) $case_msg = ' and '.$storage['contact']['name'].' as the contact person at '.$storage['org']['name'];
        drupal_set_message('Opened '.$success['case'].' cases with '.$outreach_person['name'].' as the case manager'.$case_msg.'.');
      }
      if ($success['activity']) {
        drupal_set_message('Created activity record for this trip.');
      }
    }

    if (!empty($errors)) {
      $keys = array_keys($errors);
      foreach ($keys as $error) {
        drupal_set_message('Encountered errors for the following contacts while processing <b>'.$error.'s</b>. <ul>'.implode('',$errors[$error]).'</ul>', 'error');
      }
    }
    if ($form_state['storage']['activity_id']) {
      drupal_set_message('Please send an email to the students you just met.');
      drupal_goto('staff/admissions/prospectives', 'status=1-2-3-4&trip='.$form_state['storage']['activity_id'].'&emailprompt=1');
    }
    else {
      drupal_set_message('No activity record was created');
      drupal_goto('staff/admissions/prospectives');
    }
  }
  $form_state['storage']['page']++;
}

function ws_outreach_form_validate($form, &$form_state) {
  $validate = TRUE;
  $values = $form_state['values'];

  switch($form_state['storage']['page']) {

    case 1:
      if (!is_numeric($values['number_of_students']) || $values['number_of_students'] <= 0) {
       form_set_error('number_of_students', $values['number_of_students'].'? What the heck is that?');
        $validate = FALSE;
      }
      if (!strstr($values['outreach_person'],'ID#')) {
        form_set_error('outreach_person', 'Outreach person not found in Civi Database. You will need to <a target="_blank" href="/civicrm/contact/add?ct=Individual&reset=1">add them to CiviCRM</a> before continuing.');
              $validate = FALSE;
      }
    break;

    // Page two validation
    case 2:
      if(!$form_state['values']['first_student_01'] || !$form_state['values']['first_student_01']) {
        $validate = FALSE; $i=0;
        if(!$form_state['values']['first_student_01']) {
          form_set_error('first_student_01', 'You must enter at least 1 student');
          $i++;}
        if(!$form_state['values']['last_student_01']) {
          form_set_error('last_student_01', 'You must enter at least 1 student');
          $i++;}
        if($i == 2) array_pop($_SESSION['messages']['error']); //delete extra message
        drupal_add_js(drupal_get_path('module', 'woolman_semester') .'/js/ws_admissions_2.js', 'module', 'header');
      }
    break;
  }
  if(!$validate)
    drupal_add_js(drupal_get_path('module', 'woolman_semester') .'/js/ws_admissions.js', 'module', 'header');
  return $validate;
}

/**
 * Counts successes & failures during iterative processing
 */
function ws_outreach_message($result, $type, $name, &$err, &$suc) {
  if (!empty($result['is_error'])) {
    $err[$type][] = '<li>'.$name.'; Error: "'.$result['error_message'].'"</li>';
  }
  else {
    $suc[$type]++;
  }
}

