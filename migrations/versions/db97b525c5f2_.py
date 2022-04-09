"""empty message

Revision ID: db97b525c5f2
Revises: ce41f0cba35d
Create Date: 2022-04-07 21:20:54.848038

"""
from alembic import op
import sqlalchemy as sa


# revision identifiers, used by Alembic.
revision = 'db97b525c5f2'
down_revision = 'ce41f0cba35d'
branch_labels = None
depends_on = None


def upgrade():
    # ### commands auto generated by Alembic - please adjust! ###
    op.drop_column('Appointments', 'cust_email')
    op.drop_column('Appointments', 'cust_phone')
    op.drop_column('Appointments', 'cust_name')
    # ### end Alembic commands ###


def downgrade():
    # ### commands auto generated by Alembic - please adjust! ###
    op.add_column('Appointments', sa.Column('cust_name', sa.VARCHAR(), autoincrement=False, nullable=False))
    op.add_column('Appointments', sa.Column('cust_phone', sa.VARCHAR(length=120), autoincrement=False, nullable=False))
    op.add_column('Appointments', sa.Column('cust_email', sa.VARCHAR(length=120), autoincrement=False, nullable=False))
    # ### end Alembic commands ###