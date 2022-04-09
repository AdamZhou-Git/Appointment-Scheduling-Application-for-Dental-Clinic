import os
from sqlalchemy import Column, String, Integer, create_engine
from flask_sqlalchemy import SQLAlchemy
from flask_migrate import Migrate
import json

database_path = os.environ['DATABASE_URL']

if database_path.startswith("postgres://"):
  database_path = database_path.replace("postgres://", "postgresql://", 1)
print(database_path)

db = SQLAlchemy()
#migrate = Migrate()

'''
setup_db(app)
    binds a flask application and a SQLAlchemy service
'''
def setup_db(app, database_path=database_path):
    app.config["SQLALCHEMY_DATABASE_URI"] = database_path
    app.config["SQLALCHEMY_TRACK_MODIFICATIONS"] = False
    db.app = app
    db.init_app(app)
    db.create_all()
    #migrate.init_app(app, db)


'''
Customers
Customers with attributes customer ID, name, phone, email, required service
'''
class Customers(db.Model):
    __tablename__ = 'Customers'

    customer_id = db.Column(db.Integer, primary_key=True) #generate on server
    name = db.Column(db.String, nullable=False)
    phone = db.Column(db.String(120), nullable=False)
    email = db.Column(db.String(120), nullable=False)
    required_service = db.Column(db.String(120))

    def __init__(self, name, phone, email, required_service):
        self.name = name
        self.phone = phone
        self.email = email
        self.required_service = required_service

    def insert(self):
        db.session.add(self)
        db.session.commit()

    def update(self):
        db.session.commit()

    def delete(self):
        db.session.delete(self)
        db.session.commit()

    def format(self):
        return {
            'customer_id': self.customer_id,
            'name': self.name,
            'phone': self.phone,
            'email': self.email,
            'required_service': self.required_service
        }

'''
Dentists
Dentists with attributes license ID, name, gender, specialty
'''
class Dentists(db.Model):
    __tablename__ = 'Dentists'

    license_id = db.Column(db.Integer, primary_key=True)
    name = db.Column(db.String, nullable=False)
    gender = db.Column(db.String(120), nullable=False)
    specialty = db.Column(db.String(120), nullable=False)

    def __init__(self, license_id, name, gender, specialty):
        self.license_id = license_id
        self.name = name
        self.gender = gender
        self.specialty = specialty

    def insert(self):
        db.session.add(self)
        db.session.commit()

    def update(self):
        db.session.commit()

    def delete(self):
        db.session.delete(self)
        db.session.commit()

    def format(self):
        return {
            'license_id': self.license_id,
            'name': self.name,
            'gender': self.gender,
            'specialty': self.specialty
        }


'''
Appointment
Appointments with attribute appointment ID, customer name, customer phone, customer email, 
required service, license ID, time start (designed to only have 1-hour slot, 
unique time slot for each doctor is planned to be designed in front end)
'''
class Appointmnets(db.Model):
    __tablename__ = 'Appointments'

    appt_id = db.Column(db.Integer, primary_key=True)
    cust_id = db.Column(db.Integer)
    # cust_name = db.Column(db.String, nullable=False)
    # cust_phone = db.Column(db.String(120), nullable=False)
    # cust_email = db.Column(db.String(120), nullable=False)
    service = db.Column(db.String(120))
    license_id = db.Column(db.String(120), default=False)
    start_time = db.Column(db.DateTime, nullable=False)

    def __init__(self, cust_id, service, license_id, start_time):
        self.cust_id = cust_id
        # self.name = cust_name
        # self.cust_phone = cust_phone
        # self.cust_email = cust_email
        self.service = service
        self.license_id = license_id
        self.start_time = start_time

    def insert(self):
        db.session.add(self)
        db.session.commit()

    def update(self):
        db.session.commit()

    def delete(self):
        db.session.delete(self)
        db.session.commit()

    def format(self):
        return {
            'appt_id': self.appt_id,
            'cust_id': self.cust_id,
            # 'cust_name': self.cust_name,
            # 'cust_phone': self.cust_phone,
            # 'cust_email': self.cust_email,
            'service': self.service,
            'license_id': self.license_id,
            'start_time': self.start_time
        }