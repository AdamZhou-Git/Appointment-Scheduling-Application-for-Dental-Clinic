#!/bin/bash
export DATABASE_URL="postgresql://postgres@localhost:5432/postgres"
export DATABASE_URL_TEST="postgresql://postgres@localhost:5432/dental_test"
export AUTH0_DOMAIN="dev-v6hy9z2c.us.auth0.com"
export ALGORITHMS=['RS256']
export API_AUDIENCE="DZproject"
export Clinic_Owner_token="eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCIsImtpZCI6ImgyTGIzQUVHcE9BWF8zLV9HbzZQMyJ9.eyJpc3MiOiJodHRwczovL2Rldi12Nmh5OXoyYy51cy5hdXRoMC5jb20vIiwic3ViIjoiYXV0aDB8NjI1MDZjMzlkOTg4NmQwMDcwOWFhNTQ1IiwiYXVkIjoiRFpwcm9qZWN0IiwiaWF0IjoxNjQ5NTY1MTA4LCJleHAiOjE2NDk1NzIzMDgsImF6cCI6IlJ0YWVzT01QbGpraFhrM0padktDeW11b1dHbDVsSnh6Iiwic2NvcGUiOiIiLCJwZXJtaXNzaW9ucyI6WyJkZWxldGU6YXBwb2ludG1lbnRzIiwiZGVsZXRlOmN1c3RvbWVyIiwiZGVsZXRlOmRlbnRpc3RzIiwiZ2V0OmFwcG9pbnRtZW50cyIsImdldDpjdXN0b21lcnMiLCJnZXQ6ZGVudGlzdHMiLCJwYXRjaDphcHBvaW50bWVudHMiLCJwYXRjaDpjdXN0b21lcnMiLCJwYXRjaDpkZW50aXN0cyIsInBvc3Q6YXBwb2ludG1lbnRzIiwicG9zdDpjdXN0b21lcnMiLCJwb3N0OmRlbnRpc3RzIl19.J1IZf1GX4w6K_Tmsf82FgjPEfLPgfGTopUugtS-Nqv3a0_Z8Q2qtaKu9aKDGM4vSXxcTe3H9vBa0vhvdOx6Ur5IlcRaMA1w8yOuef63gCSzZUZYiG6NumZy6Hd_qZ3B3dCNNWuXWnA8lxLpxKiqJzddBPe_JU4m1QWrowxnqJZT_y0fuhiBVV2feVMxtkOXwSgzrJSUqHjCIv24ZFhLQpGKbHDoV5rAbkAyi0dp5-krvxs_72QZY22R3srO2eb-YCffHiCjxAZDXqX60a5uwKyQBfIFe2y5isiuGRItpVcoEVBiycbg8qXjznxROsectcBK9dkd4d5wjqAG7FvIsMA"
export Customer1_token="eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCIsImtpZCI6ImgyTGIzQUVHcE9BWF8zLV9HbzZQMyJ9.eyJpc3MiOiJodHRwczovL2Rldi12Nmh5OXoyYy51cy5hdXRoMC5jb20vIiwic3ViIjoiYXV0aDB8NjI1MDZjZDZkOTg4NmQwMDcwOWFhNTc3IiwiYXVkIjoiRFpwcm9qZWN0IiwiaWF0IjoxNjQ5NTY1MTc1LCJleHAiOjE2NDk1NzIzNzUsImF6cCI6IlJ0YWVzT01QbGpraFhrM0padktDeW11b1dHbDVsSnh6Iiwic2NvcGUiOiIiLCJwZXJtaXNzaW9ucyI6WyJkZWxldGU6YXBwb2ludG1lbnRzIiwiZ2V0OmN1c3RvbWVycy1pZCIsInBhdGNoOmFwcG9pbnRtZW50cyIsInBhdGNoOmN1c3RvbWVycyIsInBvc3Q6YXBwb2ludG1lbnRzIl19.mh2_zQ1y-HXoK3DVuKj8MNylM62nDB37e82Eaadhnnpi_vrbvyD3O1r61MAj8K-jKgYdOZu4ew6KbtjWrgmZ-eboHgX8I411fsTULS0iCDTkBY4Ab06mu4qBYrbAOJbrxiLGNQF71BxUzok4084ZEqw15mGJAU6EaMPlgd3Khgb2IZa1EtzlbejC2XnGVDwtN_MyZ_bhHmRqXQoCYE_ecYXThTzDOXwNG63WEllul63uwwL4LznazQ3H9AIkWjrNVBf3yCcAKc82T3yoefH-V0Hz4wJcD4iXB0o9AvvAt4Ek23-PsRPieIiRWdb2AEY3yvHVMHKlqCWQUKMifmieSQ"
export Dentist1_token="eyJhbGciOiJSUzI1NiIsInR5cCI6IkpXVCIsImtpZCI6ImgyTGIzQUVHcE9BWF8zLV9HbzZQMyJ9.eyJpc3MiOiJodHRwczovL2Rldi12Nmh5OXoyYy51cy5hdXRoMC5jb20vIiwic3ViIjoiYXV0aDB8NjI1MDZjYTYxN2FiYjkwMDY5ZWZiNTYxIiwiYXVkIjoiRFpwcm9qZWN0IiwiaWF0IjoxNjQ5NTY1MTM3LCJleHAiOjE2NDk1NzIzMzcsImF6cCI6IlJ0YWVzT01QbGpraFhrM0padktDeW11b1dHbDVsSnh6Iiwic2NvcGUiOiIiLCJwZXJtaXNzaW9ucyI6WyJnZXQ6YXBwb2ludG1lbnRzIiwiZ2V0OmN1c3RvbWVycyIsImdldDpkZW50aXN0cyIsInBhdGNoOmFwcG9pbnRtZW50cyIsInBhdGNoOmRlbnRpc3RzIl19.eXn27usJHUkDBuaCCPs3bg5T83TNvuCaErcc8xroxqDS4BEhE7wq3K0Bq6jVR7pXbutRCgCG4KPtLXViyAAXBl42YCKJJq3DiSFwi5B7rfR_XNOpqpjbHNJXKvMNdQ-bUis26xXHo5W8iMnDBecDZY6UPcf7efPrFo84oyWZx9EG9Nkk3NEywQYRv2t4mTyAwkFhjX-xPU_Jkkb--EXPW57QdC-r6DmDmPJToop_sioJPf3G_hCtwzqhC9LFTgtjCPXTiNyHSq8R9bJE5Ouqti93V4niTO-jkYPloADOFL3q8CG3ipQxwisE859fneqQKCYi_TL3CDYhKdRegHuU8A"
echo "setup.sh script executed successfully!"