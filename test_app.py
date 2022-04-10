import os
import unittest
import json
from flask_sqlalchemy import SQLAlchemy
from flask import Flask, request, abort, jsonify, abort
from flask_migrate import Migrate
from models import setup_db, Customers, Dentists, Appointmnets
from app import create_app


DATABASE_URL_TEST = os.getenv('DATABASE_URL_TEST')
Clinic_Owner_Token = os.getenv('Clinic_Owner_token')
Dentist1_Token = os.getenv('Dentist1_token')
Customer1_Token = os.getenv('Customer1_token')


class DentalProject_TestCase(unittest.TestCase):

    def setUp(self):
        """Define test variables and initialize app."""
        self.app = create_app()
        self.client = self.app.test_client
        setup_db(self.app, DATABASE_URL_TEST)
        self.clinic_owner = Clinic_Owner_Token
        self.dentist1 = Dentist1_Token
        self.customer1 = Customer1_Token

        # binds the app to the current context
        with self.app.app_context():
            self.db = SQLAlchemy()
            self.db.init_app(self.app)
            # create all tables
            self.db.create_all()

    def tearDown(self):
        """Executed after reach test"""
        pass





# GET:
    def test_get_all_customers_no_authorization_401(self):
        res = self.client().get('/customers')
        data = json.loads(res.data)
        self.assertEqual(res.status_code, 401)
        self.assertEqual(data['Success'], False)
        self.assertEqual(data['message']['description'], 'No Authorization header is found')

    def test_get_all_customers_by_Owner(self):
        res = self.client().get('/customers',
                                headers={
                                    "Authorization": "Bearer {}"
                                    .format(self.clinic_owner)
                                })
        data = json.loads(res.data)
        self.assertEqual(res.status_code, 200)
        self.assertEqual(data['success'], True)

    def test_get_all_customers_by_dentist1(self):
        res = self.client().get('/customers',
                                headers={
                                    "Authorization": "Bearer {}"
                                    .format(self.dentist1)
                                })
        data = json.loads(res.data)
        self.assertEqual(res.status_code, 200)
        self.assertEqual(data['success'], True)

    def test_get_all_customers_by_customer1_401(self):
        res = self.client().get('/customers',
                                headers={
                                    "Authorization": "Bearer {}"
                                    .format(self.customer1)
                                })
        data = json.loads(res.data)
        self.assertEqual(res.status_code, 401)
        self.assertEqual(data['Success'], False)
        self.assertEqual(data['message']['description'], 'invalid token.')

    def test_get_own_customer_by_customer1(self):
        res = self.client().get('/customers/13',
                                headers={
                                    "Authorization": "Bearer {}"
                                    .format(self.customer1)
                                })
        data = json.loads(res.data)
        self.assertEqual(res.status_code, 200)
        self.assertEqual(data['success'], True)

    def test_get_dentists_by_owner(self):
        res = self.client().get('/dentists',
                                headers={
                                    "Authorization": "Bearer {}"
                                    .format(self.clinic_owner)
                                })
        data = json.loads(res.data)
        self.assertEqual(res.status_code, 200)
        self.assertEqual(data['success'], True)

    def test_get_dentists_by_dentist1(self):
        res = self.client().get('/dentists',
                                headers={
                                    "Authorization": "Bearer {}"
                                    .format(self.dentist1)
                                })
        data = json.loads(res.data)
        self.assertEqual(res.status_code, 200)
        self.assertEqual(data['success'], True)

    def test_get_dentists_by_customer1_401(self):
        res = self.client().get('/dentists',
                                headers={
                                    "Authorization": "Bearer {}"
                                    .format(self.customer1)
                                })
        data = json.loads(res.data)
        self.assertEqual(res.status_code, 401)
        self.assertEqual(data['Success'], False)
        self.assertEqual(data['message']['description'], 'invalid token.')

    def test_get_all_appointments_by_owner(self):
        res = self.client().get('/appointments',
                                headers={
                                    "Authorization": "Bearer {}"
                                    .format(self.clinic_owner)
                                })
        data = json.loads(res.data)
        self.assertEqual(res.status_code, 200)
        self.assertEqual(data['success'], True)

    def test_get_all_appointments_by_dentist1(self):
        res = self.client().get('/appointments',
                                headers={
                                    "Authorization": "Bearer {}"
                                    .format(self.dentist1)
                                })
        data = json.loads(res.data)
        self.assertEqual(res.status_code, 200)
        self.assertEqual(data['success'], True)

    def test_get_all_appointments_by_customer1_401(self):
        res = self.client().get('/appointments',
                                headers={
                                    "Authorization": "Bearer {}"
                                    .format(self.customer1)
                                })
        data = json.loads(res.data)
        self.assertEqual(res.status_code, 401)
        self.assertEqual(data['Success'], False)
        self.assertEqual(data['message']['description'], 'invalid token.')



# POST and Delete:
    def test_delete_specific_customer_by_owner(self):
        customer_delete = Customers.query.filter(Customers.name == "Mary C. Kenyon").one_or_none()
        customer_id_delete = customer_delete.customer_id

        res = self.client().delete('/customers/'+str(customer_id_delete),
                                headers={
                                    "Authorization": "Bearer {}"
                                    .format(self.clinic_owner)
                                })
        data = json.loads(res.data)
        self.assertEqual(res.status_code, 200)
        self.assertEqual(data['success'], True)


    def test_post_customer_by_owner(self):
        new_customer = {
                        "name": "Mary C. Kenyon",
                        "phone": "210-659-5245",
                        "email":"MaryCKenyon@armyspy.com",
                        "required_service":"Dental Services"
        }

        res = self.client().post('/customers',
                                headers={
                                    "Authorization": "Bearer {}"
                                    .format(self.clinic_owner)
                                }, json = new_customer)
        data = json.loads(res.data)
        self.assertEqual(res.status_code, 200)
        self.assertEqual(data['success'], True)


    def test_post_customer_by_dentist1_401(self):
        new_customer = {
            "name": "fake customer",
            "phone": "111-111-1111",
            "email": "fakeemial@email.com",
            "required_service": "Dental Services"
        }

        res = self.client().post('/customers',
                                 headers={
                                     "Authorization": "Bearer {}"
                                 .format(self.dentist1)
                                 }, json=new_customer)
        data = json.loads(res.data)
        self.assertEqual(res.status_code, 401)
        self.assertEqual(data['Success'], False)
        self.assertEqual(data['message']['description'], 'invalid token.')

    def test_delete_specific_dentist_by_owner(self):
        dentist_delete = Dentists.query.filter(Dentists.name == "Flora R. Payne").one_or_none()
        license_id_delete = dentist_delete.license_id

        res = self.client().delete('/dentists/'+str(license_id_delete),
                                headers={
                                    "Authorization": "Bearer {}"
                                    .format(self.clinic_owner)
                                })
        data = json.loads(res.data)
        self.assertEqual(res.status_code, 200)
        self.assertEqual(data['success'], True)


    def test_post_dentist_by_owner(self):
        new_dentist = {
            "license_id": "20",
            "name": "Flora R. Payne",
            "gender": "male",
            "specialty": "Dental Services"
        }

        res = self.client().post('/dentists',
                                headers={
                                    "Authorization": "Bearer {}"
                                    .format(self.clinic_owner)
                                }, json = new_dentist)
        data = json.loads(res.data)
        self.assertEqual(res.status_code, 200)
        self.assertEqual(data['success'], True)

    def test_post_dentist_by_customer1_401(self):
        new_dentist = {
            "license_id": "20",
            "name": "Flora R. Payne",
            "gender": "male",
            "specialty": "Dental Services"
        }

        res = self.client().post('/dentists',
                                headers={
                                    "Authorization": "Bearer {}"
                                    .format(self.customer1)
                                }, json = new_dentist)
        data = json.loads(res.data)
        self.assertEqual(res.status_code, 401)
        self.assertEqual(data['Success'], False)
        self.assertEqual(data['message']['description'], 'invalid token.')

    def test_post_dentist_by_dentist1_401(self):
        new_dentist = {
            "license_id": "20",
            "name": "Flora R. Payne",
            "gender": "male",
            "specialty": "Dental Services"
        }

        res = self.client().post('/dentists',
                                headers={
                                    "Authorization": "Bearer {}"
                                    .format(self.dentist1)
                                }, json = new_dentist)
        data = json.loads(res.data)
        self.assertEqual(res.status_code, 401)
        self.assertEqual(data['Success'], False)
        self.assertEqual(data['message']['description'], 'invalid token.')



    def test_delete_specific_appointment_by_customer1(self):
        customer1 = Customers.query.filter(Customers.name == "Randy L. Cox").one_or_none()
        customer_id = customer1.customer_id
        appt = Appointmnets.query.filter(Appointmnets.cust_id == customer_id).first()
        appt_id = appt.appt_id

        res = self.client().delete('/appointments/'+str(appt_id),
                                headers={
                                    "Authorization": "Bearer {}"
                                    .format(self.customer1)
                                })
        data = json.loads(res.data)
        self.assertEqual(res.status_code, 200)
        self.assertEqual(data['success'], True)


    def test_post_appointment_by_customer1(self):
        customer1 = Customers.query.filter(Customers.name == "Randy L. Cox").one_or_none()
        customer_id = customer1.customer_id

        new_appointment = {
                "id": customer_id,
                "service": "Orthodontic Services",
                "start_time": "2023-02-14 14:00:00.000000"
        }

        res = self.client().post('/appointments',
                                headers={
                                    "Authorization": "Bearer {}"
                                    .format(self.customer1)
                                }, json = new_appointment)
        data = json.loads(res.data)
        self.assertEqual(res.status_code, 200)
        self.assertEqual(data['success'], True)


# PATCH
    def test_patch_appointment_by_customer1(self):
        customer1 = Customers.query.filter(Customers.name == "Timothy J. Kincaid").one_or_none()
        customer_id = customer1.customer_id
        appt = Appointmnets.query.filter(Appointmnets.cust_id == customer_id).first()
        appt_id = appt.appt_id

        updated_appointment = {
                "service": "Orthodontic Services",
                "start_time": "2022-08-15 14:00:00.000000"
        }

        res = self.client().patch('/appointments/'+str(appt_id),
                                headers={
                                    "Authorization": "Bearer {}"
                                    .format(self.customer1)
                                }, json = updated_appointment)
        data = json.loads(res.data)
        self.assertEqual(res.status_code, 200)
        self.assertEqual(data['success'], True)


    def test_patch_customer_by_customer1(self):
        customer1 = Customers.query.filter(Customers.name == "Timothy J. Kincaid").one_or_none()
        customer_id = customer1.customer_id

        updated_customer_info = {
            "name": "Timothy J. Kincaid",
            "phone": "000-000-0000",
            "email": "updatedemail@gamil.com",
            "required_service":"Orthodontic Services,Dental Services"
        }

        res = self.client().patch('/customers/'+str(customer_id),
                                headers={
                                    "Authorization": "Bearer {}"
                                    .format(self.customer1)
                                }, json = updated_customer_info)
        data = json.loads(res.data)
        self.assertEqual(res.status_code, 200)
        self.assertEqual(data['success'], True)


    def test_patch_dentist_by_dentist1(self):
        dentist1 = Dentists.query.filter(Dentists.name == "Wang Erma").one_or_none()
        license_id = dentist1.license_id

        updated_dentist_info = {
                "license_id": "3",
                "name": "Wang Erma",
                "gender":"male",
                "specialty":"Updated service"
        }

        res = self.client().patch('/dentists/'+str(license_id),
                                headers={
                                    "Authorization": "Bearer {}"
                                    .format(self.dentist1)
                                }, json = updated_dentist_info)
        data = json.loads(res.data)
        self.assertEqual(res.status_code, 200)
        self.assertEqual(data['success'], True)


if __name__ == "__main__":
    unittest.main()