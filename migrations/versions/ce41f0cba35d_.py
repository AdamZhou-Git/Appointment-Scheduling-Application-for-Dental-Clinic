"""empty message

Revision ID: ce41f0cba35d
Revises: 
Create Date: 2022-04-07 19:48:48.465120

"""
from alembic import op
import sqlalchemy as sa


# revision identifiers, used by Alembic.
revision = 'ce41f0cba35d'
down_revision = None
branch_labels = None
depends_on = None


def upgrade():
    # ### commands auto generated by Alembic - please adjust! ###
    op.create_table('Appointments',
    sa.Column('appt_id', sa.Integer(), nullable=False),
    sa.Column('cust_id', sa.Integer(), nullable=True),
    sa.Column('cust_name', sa.String(), nullable=False),
    sa.Column('cust_phone', sa.String(length=120), nullable=False),
    sa.Column('cust_email', sa.String(length=120), nullable=False),
    sa.Column('service', sa.String(length=120), nullable=True),
    sa.Column('license_id', sa.String(length=120), nullable=True),
    sa.Column('start_time', sa.DateTime(), nullable=False),
    sa.PrimaryKeyConstraint('appt_id')
    )
    op.create_table('Customers',
    sa.Column('customer_id', sa.Integer(), nullable=False),
    sa.Column('name', sa.String(), nullable=False),
    sa.Column('phone', sa.String(length=120), nullable=False),
    sa.Column('email', sa.String(length=120), nullable=False),
    sa.Column('required_service', sa.String(length=120), nullable=True),
    sa.PrimaryKeyConstraint('customer_id')
    )
    op.create_table('Dentists',
    sa.Column('license_id', sa.Integer(), nullable=False),
    sa.Column('name', sa.String(), nullable=False),
    sa.Column('gender', sa.String(length=120), nullable=False),
    sa.Column('specialty', sa.String(length=120), nullable=False),
    sa.PrimaryKeyConstraint('license_id')
    )
    # ### end Alembic commands ###


def downgrade():
    # ### commands auto generated by Alembic - please adjust! ###
    op.drop_table('Dentists')
    op.drop_table('Customers')
    op.drop_table('Appointments')
    # ### end Alembic commands ###
