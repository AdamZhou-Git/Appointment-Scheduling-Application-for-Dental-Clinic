import os
from flask import Flask, request, abort, jsonify, abort
from flask_sqlalchemy import SQLAlchemy
import json
from flask_cors import CORS
from sqlalchemy import func

from models import setup_db, Customers, Dentists, Appointmnets
from auth import AuthError, requires_auth


def create_app(test_config=None):
    # create and configure the app
    app = Flask(__name__)
    setup_db(app)
    CORS(app)

    '''
    Use the after_request decorator to set Access-Control-Allow
    '''
    @app.after_request
    def after_request(response):
        response.headers.add('Access-Control-Allow-Headers', 'Content-Type,Authorization,true')
        response.headers.add('Access-Control-Allow-Methods', 'Get,PUT,POST,DELETE,OPTIONS')
        return response

    items_per_page = 25

    def paginate_items(request, selection):
        page = request.args.get('page', 1, type=int)
        start = (page - 1) * items_per_page
        end = start + items_per_page

        items = [item for item in selection]
        current_list_items = items[start:end]

        return current_list_items

    @app.route('/')
    def index_endpoint():
        return "For Udacity Capstone Project: " \
               "please loin to https://dev-v6hy9z2c.us.auth0.com/authorize?audience=DZproject&response_type=token&client_id=RtaesOMPljkhXk3JZvKCymuoWGl5lJxz&redirect_uri=http://localhost:8080/login-results " \
               " | Customer login: customer1@email.com; password: customer1@email.com" \
               " | Dentist login:  dentist1@email.com; password: dentist1@email.com" \
               " | Clinic Owner login: clinicowner@email.com; password: ClinicOwner@email.com"

# ROUTES : GET
    '''
    GET /customers
    '''
    @app.route('/customers')
    @requires_auth('get:customers')
    def get_customers(jwt):
        customers = Customers.query.all()
        total_customers = len(customers)
        curr_page_customer = paginate_items(request, customers)

        if len(curr_page_customer) == 0:
          abort(404)

        return jsonify(
            {
                'success': True,
                'customers': [x.format() for x in curr_page_customer],
                'total_customers': total_customers
            }
        ), 200

    '''
    GET /customers/id; this is used for customer login page, which shows customer info and their appointments
    '''
    @app.route('/customers/<int:id>', methods=['GET'])
    @requires_auth('get:customers-id')
    def get_customers_byID(jwt, id):
        customer = Customers.query.filter(Customers.customer_id == id).one_or_none()
        upcoming_appt = Appointmnets.query.filter(Appointmnets.cust_id == id
                                                ,Appointmnets.start_time >= func.CURRENT_DATE())

        if customer is None:
            abort(404)

        return jsonify(
            {
                'success': True,
                'customer': customer.format(),
                'upcoming_appt': [x.format() for x in upcoming_appt]
            }
        ), 200

    '''
    GET /dentists
    '''
    @app.route('/dentists')
    @requires_auth('get:dentists')
    def get_dentists(jwt):
        dentists = Dentists.query.all()
        total_dentists = len(dentists)
        curr_page_dentists = paginate_items(request, dentists)

        if len(curr_page_dentists) == 0:
            abort(404)

        return jsonify(
            {
                'success': True,
                'dentists': [x.format() for x in curr_page_dentists],
                'total_dentists': total_dentists
            }
        ), 200

    '''
    GET /appointments
    '''
    @app.route('/appointments')
    @requires_auth('get:appointments')
    def get_appointments(jwt):
        appointments = Appointmnets.query.all()
        total_appointments = len(appointments)
        curr_page_appointments = paginate_items(request, appointments)

        if len(curr_page_appointments) == 0:
            abort(404)

        return jsonify(
            {
                'success': True,
                'appointments': [x.format() for x in curr_page_appointments] ,
                'total_appointments': total_appointments
            }
        ), 200




# ROUTES : DELETE
    @app.route('/customers/<int:id>', methods=['DELETE'])
    @requires_auth('delete:customer')
    def delete_customer(jwt, id):
        customer_delete = Customers.query.filter(Customers.customer_id == id).one_or_none()

        if customer_delete  is None:
            abort(404)

        customer_delete.delete()

        customers = Customers.query.all()
        total_customers = len(customers)
        curr_page_customer = paginate_items(request, customers)

        return jsonify(
            {
                'success': True,
                'delete': id,
                'customers': [x.format() for x in curr_page_customer],
                'total_customers': total_customers
            }
        ), 200

    @app.route('/dentists/<int:id>', methods=['DELETE'])
    @requires_auth('delete:dentists')
    def delete_dentists(jwt, id):
        dentists_delete = Dentists.query.filter(Dentists.license_id == id).one_or_none()

        if dentists_delete is None:
            abort(404)

        dentists_delete.delete()

        dentists = Dentists.query.all()
        total_dentists = len(dentists)
        curr_page_dentists = paginate_items(request, dentists)

        return jsonify(
            {
                'success': True,
                'delete': id,
                'dentists': [x.format() for x in curr_page_dentists],
                'total_dentists': total_dentists

            }
        ), 200

    @app.route('/appointments/<int:id>', methods=['DELETE'])
    @requires_auth('delete:appointments')
    def delete_appointments(jwt, id):
        appointments_delete = Appointmnets.query.filter(Appointmnets.appt_id == id).one_or_none()

        if appointments_delete is None:
            abort(404)

        appointments_delete.delete()

        appointments = Appointmnets.query.all()
        total_appointments = len(appointments)
        curr_page_appointments = paginate_items(request, appointments)

        return jsonify(
            {
                'success': True,
                'delete': id,
                'appointments': [x.format() for x in curr_page_appointments],
                'total_appointments': total_appointments
            }
        ), 200


# ROUTES : POST
    @app.route('/customers', methods=['POST'])
    @requires_auth('post:customers')
    def create_customers(jwt):
        body = request.get_json()
        new_name = body.get('name')
        new_phone = body.get('phone')
        new_email = body.get('email')
        new_required_service = body.get('required_service')

        if (new_name is None) or (new_email is None) or (new_phone is None):
            abort(422)

        new_customer = Customers(name = new_name, phone = new_phone,
                                 email = new_email, required_service = new_required_service)
        new_customer.insert()

        return jsonify(
            {
                "success": True,
                "new_customer": new_customer.format()
            }
        ), 200

    @app.route('/dentists', methods=['POST'])
    @requires_auth('post:dentists')
    def create_dentists(jwt):
        body = request.get_json()
        new_license_id = body.get('license_id')
        new_name = body.get('name')
        new_gender = body.get('gender')
        new_specialty = body.get('specialty')

        if (new_name is None) or (new_gender is None) or (new_specialty is None) or (new_license_id is None):
            abort(422)

        new_dentist = Dentists(license_id= new_license_id, name = new_name, gender= new_gender, specialty = new_specialty)

        new_dentist.insert()

        return jsonify(
            {
                "success": True,
                "new_dentist": new_dentist.format()
            }
        ), 200

    @app.route('/appointments', methods=['POST'])
    @requires_auth('post:appointments')
    def create_appointments(jwt):
        body = request.get_json()
        # new_name = body.get('name')
        # new_phone = body.get('phone')
        # new_email = body.get('email')
        new_service = body.get('service')
        new_start_time = body.get('start_time') #unique time is achieved at front end
        license_id = None #None as default value
        cust_id = body.get('id')

        if None in (new_start_time, cust_id):
            abort(422)

        new_appt = Appointmnets(cust_id = cust_id,
                                # cust_name = new_name, cust_phone = new_phone, cust_email = new_email,
                                service = new_service, start_time = new_start_time, license_id = license_id)
        new_appt.insert()

        return jsonify(
            {
                "success": True,
                "new_appointment": new_appt.format()
            }
        ), 200


# ROUTES : PATCH
    @app.route('/customers/<int:id>', methods=['PATCH'])
    @requires_auth('patch:customers')
    def patch_customers(jwt, id):
        customer_patch = Customers.query.filter(Customers.customer_id == id).one_or_none()

        if customer_patch is None:
            abort(404)

        body = request.get_json()
        new_name = body.get('name')
        new_phone = body.get('phone')
        new_email = body.get('email')
        new_required_service = body.get('required_service')

        if new_name:
            customer_patch.name = new_name
        if new_phone:
            customer_patch.phone = new_phone
        if new_email:
            customer_patch.email = new_email
        if new_required_service:
            customer_patch.required_service = new_required_service

        customer_patch.update()

        return jsonify(
            {
                "success": True,
                "updated_customer": customer_patch.format()
            }
        ), 200


    @app.route('/dentists/<int:id>', methods=['PATCH'])
    @requires_auth('patch:dentists')
    def patch_dentists(jwt, id):
        dentists_patch = Dentists.query.filter(Dentists.license_id == id).one_or_none()

        if dentists_patch is None:
            abort(404)

        body = request.get_json()
        new_license_id = body.get('license_id')
        new_name = body.get('name')
        new_gender = body.get('gender')
        new_specialty = body.get('specialty')

        if new_name:
            dentists_patch.name = new_name
        if new_license_id:
            dentists_patch.license_id = new_license_id
        if new_gender:
            dentists_patch.gender = new_gender
        if new_specialty:
            dentists_patch.specialty = new_specialty

        dentists_patch.update()

        return jsonify(
            {
                "success": True,
                "updated_dentists": dentists_patch.format()

            }
        ), 200


    @app.route('/appointments/<int:id>', methods=['PATCH'])
    @requires_auth('patch:appointments')
    def patch_appointments(jwt, id):
        appointments_patch = Appointmnets.query.filter(Appointmnets.appt_id == id).one_or_none()

        if appointments_patch is None:
            abort(404)

        body = request.get_json()
        # new_name = body.get('name')
        # new_phone = body.get('phone')
        # new_email = body.get('email')
        new_service = body.get('service')
        new_start_time = body.get('start_time')

        # if new_name:
        #     appointments_patch.cust_name = new_name
        # if new_phone:
        #     appointments_patch.cust_phone = new_phone
        # if new_email:
        #     appointments_patch.cust_email = new_email
        if new_service:
            appointments_patch.service = new_service
        if new_start_time:
            appointments_patch.start_time = new_start_time

        appointments_patch.update()

        return jsonify(
            {
                "success": True,
                "updated_appt": appointments_patch.format()
            }
        ), 200


# error handler
    @app.errorhandler(422)
    def unprocessable(error):
        return jsonify({
            "success": False,
            "error": 422,
            "message": "unprocessable"
        }), 422

    @app.errorhandler(404)
    def not_found(error):
        return jsonify({
            "success": False,
            "error": 404,
            "message": "resource not found"
        }), 404

    @app.errorhandler(AuthError)
    def AuthError_unprocessable(error):
        return jsonify({
            "Success": False,
            "error": error.status_code,
            "message": error.error
        }), 401


    return app

app = create_app()

if __name__ == '__main__':
    app.run(host='0.0.0.0', port=8080, debug=True)